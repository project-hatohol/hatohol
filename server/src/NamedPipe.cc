/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib-object.h>

#include <string>
using namespace std;

#include <SmartBuffer.h>
#include <MutexLock.h>
using namespace mlpl;

#include "HatoholException.h"
#include "NamedPipe.h"
#include "Utils.h"

const char *NamedPipe::BASE_DIR = "/tmp/hatohol";
unsigned BASE_DIR_MODE =
   S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP;
unsigned FIFO_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

typedef list<SmartBuffer *>       SmartBufferList;
typedef SmartBufferList::iterator SmartBufferListIterator;

struct TimeoutInfo {
	NamedPipe                 *namedPipe;
	guint                      tag;
	unsigned long              value; // millisecond
	NamedPipe::TimeoutCallback cbFunc;
	void                      *priv;

	TimeoutInfo(NamedPipe *pipe)
	: namedPipe(pipe),
	  tag(INVALID_EVENT_ID),
	  value(0),
	  cbFunc(NULL),
	  priv(NULL)
	{
	}

	virtual ~TimeoutInfo()
	{
		Utils::removeEventSourceIfNeeded(tag);
	}

	static gboolean timeoutHandler(gpointer data)
	{
		TimeoutInfo *obj = static_cast<TimeoutInfo *>(data);
		NamedPipe *namedPipe = obj->namedPipe;

		// Setting obj->tag has to be put before excuting the callback
		// handler. If the callback deletes the NamedPipe instance
		// that has this instance, it is an invalid memory access.
		// Even if the callback just makes a trigger of the deletion,
		// it's also possible.
		// For example, ActionManager::residentActionTimeoutCb(),
		// a user of this timeout callback mechanism, sends SIGKILL.
		// Then ActionManager::residentActorCollectedCb() called from
		// the ActorCollector thread deletes a ResidentInfo instance
		// that has a NamedPipe instance and this instance.
		// In that case, the problem may happen, especially on a single
		// core CPU.
		obj->tag = INVALID_EVENT_ID;
		(*obj->cbFunc)(namedPipe, obj->priv);
		return FALSE;
	}

	void setTimeoutIfNeeded(void)
	{
		if (!cbFunc)
			return;
		if (tag != INVALID_EVENT_ID)
			return;
		tag = g_timeout_add(value, timeoutHandler, this);
	}

	void removeTimeout(void)
	{
		Utils::removeEventSourceIfNeeded(tag);
		tag = INVALID_EVENT_ID;
	}
};

struct NamedPipe::PrivateContext {
	int fd;
	string path;
	EndType endType;
	GIOChannel *ioch;
	guint iochEvtId;
	guint iochDataEvtId;
	GIOFunc  userCb;
	gpointer userCbData;
	bool writeCbSet;
	SmartBufferList writeBufList;
	MutexLock writeBufListLock;
	PullCallback  pullCb;
	void         *pullCbPriv;
	SmartBuffer   pullBuf;
	size_t        pullRequestSize;
	size_t        pullRemainingSize;
	TimeoutInfo   timeoutInfo;

	PrivateContext(EndType _endType, NamedPipe *namedPipe)
	: fd(-1),
	  endType(_endType),
	  ioch(NULL),
	  iochEvtId(INVALID_EVENT_ID),
	  iochDataEvtId(INVALID_EVENT_ID),
	  userCb(NULL),
	  userCbData(NULL),
	  pullCb(NULL),
	  pullCbPriv(NULL),
	  pullRequestSize(0),
	  pullRemainingSize(0),
	  timeoutInfo(namedPipe)
	{
	}

	virtual ~PrivateContext()
	{
		Utils::removeEventSourceIfNeeded(iochEvtId);
		Utils::removeEventSourceIfNeeded(iochDataEvtId);

		// After an error occurred, g_io_channel_shutdown() fails.
		// So we check iochEvtId to know if an error occurred.
		if (ioch && (iochEvtId != INVALID_EVENT_ID)) {
			const gboolean flush = FALSE;
			GError *error = NULL;
			GIOStatus stat
			  = g_io_channel_shutdown(ioch, flush, &error);
			if (stat != G_IO_STATUS_NORMAL) {
				MLPL_ERR("Failed to shutdown. status: %d, %s\n",
				         stat, error ?
				         error->message : "unknown reason");
				if (error)
					g_error_free(error);
			}
		}
		if (ioch)
			g_io_channel_unref(ioch);
		if (fd >= 0)
			close(fd);
		SmartBufferListIterator it = writeBufList.begin();
		for (; it != writeBufList.end(); ++it)
			delete *it;
	}

	void closeFd(void)
	{
		if (fd < 0) {
			MLPL_WARN("closeFd() is called when fd = %d\n", fd);
			return;
		}
		close(fd);
		fd = -1;
	}

	void deleteWriteBufHead(void)
	{
		HATOHOL_ASSERT(!writeBufList.empty(), "Buffer is empty.");
		delete writeBufList.front();
		writeBufList.pop_front();
	}

	void callPullCb(GIOStatus stat, PullCallback altCb = NULL)
	{
		PullCallback cbFunc = altCb ? altCb : pullCb;;
		pullBuf.resetIndex();
		(*cbFunc)(stat, pullBuf, pullRequestSize, pullCbPriv);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
NamedPipe::NamedPipe(EndType endType)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(endType, this);
}

NamedPipe::~NamedPipe()
{
	if (m_ctx)
		delete m_ctx;
}

bool NamedPipe::init(const string &name, GIOFunc iochCb, gpointer data)
{
	if (!openPipe(name))
		return false;

	m_ctx->ioch = g_io_channel_unix_new(m_ctx->fd);
	if (!m_ctx->ioch) {
		MLPL_ERR("Failed to call g_io_channel_unix_new: %d\n",
		         m_ctx->fd);
		return false;
	}
	GError *error = NULL;
	GIOStatus stat = g_io_channel_set_encoding(m_ctx->ioch, NULL, &error);
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Failed to call g_io_channel_set_encoding: "
		         "%d, %s\n", stat,
		         error ? error->message : "(unknown reason)");
		return false;
	}

	GIOFunc cbFunc = NULL;
	GIOCondition cond = (GIOCondition)0;
	if (m_ctx->endType == END_TYPE_MASTER_READ
	    || m_ctx->endType == END_TYPE_SLAVE_READ) {
		cbFunc = readErrorCb;
		cond = (GIOCondition)(G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL);
	}
	else if (m_ctx->endType == END_TYPE_MASTER_WRITE
	         || m_ctx->endType == END_TYPE_SLAVE_WRITE) {
		cbFunc = writeErrorCb;
		cond = (GIOCondition)(G_IO_ERR|G_IO_HUP|G_IO_NVAL);
	}
	m_ctx->iochEvtId = g_io_add_watch(m_ctx->ioch, cond, cbFunc, this);

	m_ctx->userCb = iochCb;
	m_ctx->userCbData = data;

	return true;
}

int NamedPipe::getFd(void) const
{
	return m_ctx->fd;
}

const string &NamedPipe::getPath(void) const
{
	return m_ctx->path;
}

void NamedPipe::push(SmartBuffer &buf)
{
	// <<Note>>
	// This function is possibly called from threads other than
	// the main thread (that is executing the GLIB's event loop).
	HATOHOL_ASSERT(m_ctx->endType == END_TYPE_MASTER_WRITE
	               || m_ctx->endType == END_TYPE_SLAVE_WRITE,
	               "push() can be called only by writers: %d\n",
	               m_ctx->endType);
	buf.resetIndex();
	m_ctx->writeBufListLock.lock();
	m_ctx->writeBufList.push_back(buf.takeOver());
	enableWriteCbIfNeeded();
	m_ctx->timeoutInfo.setTimeoutIfNeeded();
	m_ctx->writeBufListLock.unlock();
}

void NamedPipe::pull(size_t size, PullCallback callback, void *priv)
{
	HATOHOL_ASSERT(!m_ctx->pullCb, "Pull callback is not NULL.");
	m_ctx->pullRequestSize = size;
	m_ctx->pullRemainingSize = size;
	m_ctx->pullCb = callback;
	m_ctx->pullCbPriv = priv;
	m_ctx->pullBuf.ensureRemainingSize(size);
	m_ctx->pullBuf.resetIndex();
	enableReadCb();
	m_ctx->timeoutInfo.setTimeoutIfNeeded();
}

void NamedPipe::setTimeout(unsigned int timeout,
                           TimeoutCallback timeoutCb, void *priv)
{
	Utils::removeEventSourceIfNeeded(m_ctx->timeoutInfo.tag);
	if (timeout == 0)
		return;
	if (!timeoutCb) {
		MLPL_ERR("Timeout callback is NULL\n");
		return;
	}
	m_ctx->timeoutInfo.value  = timeout;
	m_ctx->timeoutInfo.cbFunc = timeoutCb;
	m_ctx->timeoutInfo.priv   = priv;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gboolean NamedPipe::writeCb(GIOChannel *source, GIOCondition condition,
                            gpointer data)
{
	NamedPipe *obj = static_cast<NamedPipe *>(data);
	PrivateContext *ctx = obj->m_ctx;
	gboolean continueEventCb = FALSE;

	ctx->writeBufListLock.lock();
	gint currOutEvtId = ctx->iochDataEvtId;
	ctx->iochDataEvtId = INVALID_EVENT_ID;
	if (ctx->writeBufList.empty()) {
		MLPL_BUG("writeCB was called. "
		         "However, write buffer list is empty.\n");
	}
	while (!ctx->writeBufList.empty()) {
		SmartBufferListIterator it = ctx->writeBufList.begin();
		bool fullyWritten = false;
		if (!obj->writeBuf(**it, fullyWritten))
			break;
		if (fullyWritten) {
			ctx->deleteWriteBufHead();
			ctx->timeoutInfo.removeTimeout();
		} else {
			// This function will be called back again
			// when the pipe is available.
			ctx->iochDataEvtId = currOutEvtId;
			continueEventCb = TRUE;

			// If this is the first chunk for each push request,
			// the timeout will be enabled in the following
			// function. Otherwise, the timeout is expected to
			// be already set. In that case, the following function
			// won't update the timer,
			ctx->timeoutInfo.setTimeoutIfNeeded();
			break;
		}
	}
	ctx->writeBufListLock.unlock();
	return continueEventCb;
}

gboolean NamedPipe::writeErrorCb(GIOChannel *source, GIOCondition condition,
                                 gpointer data)
{
	NamedPipe *obj = static_cast<NamedPipe *>(data);
	PrivateContext *ctx = obj->m_ctx;
	HATOHOL_ASSERT(ctx->userCb,
	               "Pull callback is not registered. cond: %x, ctx: %p",
	               condition, ctx);
	ctx->iochEvtId = INVALID_EVENT_ID;
	return (*ctx->userCb)(source, condition, ctx->userCbData);
}

gboolean NamedPipe::readCb(GIOChannel *source, GIOCondition condition,
                           gpointer data)
{
	NamedPipe *obj = static_cast<NamedPipe *>(data);
	PrivateContext *ctx = obj->m_ctx;
	HATOHOL_ASSERT(ctx->pullCb,
	               "Pull callback is not registered. cond: %x, ctx: %p",
	               condition, ctx);

	gsize bytesRead;
	GError *error = NULL;
	gchar *buf = ctx->pullBuf.getPointer<gchar>();
	GIOStatus stat = g_io_channel_read_chars(source, buf,
	                                         ctx->pullRemainingSize,
	                                         &bytesRead, &error);
	if (!obj->checkGIOStatus(stat, error)) {
		ctx->timeoutInfo.removeTimeout();
		ctx->callPullCb(stat);
		ctx->iochDataEvtId = INVALID_EVENT_ID;
		return FALSE;
	}
	
	ctx->pullRemainingSize -= bytesRead;
	if (ctx->pullRemainingSize == 0) {
		ctx->iochDataEvtId = INVALID_EVENT_ID;

		// backup the current callback function
		PullCallback callback = ctx->pullCb;

		// We set NULL to avoid HATOHOL_ASSERTION in pull() from
		// invoking when the pull() is called in the callback.
		ctx->pullCb = NULL;
		ctx->callPullCb(stat, callback);
		return FALSE;
	}

	return TRUE;
}

gboolean NamedPipe::readErrorCb(GIOChannel *source, GIOCondition condition,
                                gpointer data)
{
	NamedPipe *obj = static_cast<NamedPipe *>(data);
	PrivateContext *ctx = obj->m_ctx;
	HATOHOL_ASSERT(ctx->userCb,
	               "Pull callback is not registered. cond: %x, ctx: %p",
	               condition, ctx);
	ctx->iochEvtId = INVALID_EVENT_ID;
	return (*ctx->userCb)(source, condition, ctx->userCbData);
}

bool NamedPipe::openPipe(const string &name)
{
	HATOHOL_ASSERT(m_ctx->fd == -1,
	               "FD must be -1 (%d). NamedPipe::open() is possibly "
	               "called multiple times.\n", m_ctx->fd);
	if (!makeBasedirIfNeeded(BASE_DIR))
		return false;

	int suffix;
	int openFlag;
	bool recreate = false;
	// We assume that this class is used as a master-slave model.
	// The master first makes and open pipes. Then the slave opens them.
	// We use O_NONBLOCK and O_RDWR to avoid the open by the master
	// from blocking.
	// 
	// NOTE:
	// The behavior of O_RDWR for the pipe is not specified in POSIX.
	// It works without blocking on Linux.
	switch (m_ctx->endType) {
	case END_TYPE_MASTER_READ:
		suffix = 0;
		openFlag = O_RDONLY|O_NONBLOCK;
		recreate = true;
		break;
	case END_TYPE_MASTER_WRITE:
		suffix = 1;
		openFlag = O_RDWR|O_NONBLOCK;
		recreate = true;
		break;
	case END_TYPE_SLAVE_READ:
		suffix = 1;
		openFlag = O_RDONLY;
		break;
	case END_TYPE_SLAVE_WRITE:
		suffix = 0;
		openFlag = O_WRONLY|O_NONBLOCK;
		break;
	default:
		HATOHOL_ASSERT(false, "Invalid endType: %d\n", m_ctx->endType);
	}
	m_ctx->path = StringUtils::sprintf("%s/%s-%d",
	                                   BASE_DIR, name.c_str(), suffix);
	if (recreate) {
		if (!deleteFileIfExists(m_ctx->path))
			return false;
		if (mkfifo(m_ctx->path.c_str(), FIFO_MODE) == -1) { 
			MLPL_ERR("Failed to make FIFO: %s, %s\n",
			         m_ctx->path.c_str(), strerror(errno));
			return false;
		}
	}

	// open the fifo
retry:
	m_ctx->fd = open(m_ctx->path.c_str(), openFlag);
	if (m_ctx->fd == -1) {
		if (errno == EINTR)
			goto retry;
		MLPL_ERR("Failed to open: %s, %s\n",
		         m_ctx->path.c_str(), strerror(errno));
		return false;
	}

	return true;
}

bool NamedPipe::writeBuf(SmartBuffer &buf, bool &fullyWritten, bool flush)
{
	fullyWritten = false;
	gchar *dataPtr = buf.getPointer<gchar>();
	gssize count = buf.watermark() - buf.index();
	gsize bytesWritten;
	GError *error = NULL;
	GIOStatus stat =
	  g_io_channel_write_chars(m_ctx->ioch, dataPtr, count,
	                           &bytesWritten, &error);
	if (!checkGIOStatus(stat, error))
		return false;
	if (flush) {
		stat = g_io_channel_flush(m_ctx->ioch, &error);
		if (!checkGIOStatus(stat, error))
			return false;
	}
	buf.incIndex(bytesWritten);
	if (bytesWritten == (gsize)count)
		fullyWritten = true;
	return true;
}

void NamedPipe::enableWriteCbIfNeeded(void)
{
	// We assume that this function is called
	// with the lock of m_ctx->writeBufListLock.
	if (m_ctx->iochDataEvtId != INVALID_EVENT_ID)
		return;
	GIOCondition cond = G_IO_OUT;
	m_ctx->iochDataEvtId = g_io_add_watch(m_ctx->ioch, cond, writeCb, this);
}

void NamedPipe::enableReadCb(void)
{
	GIOCondition cond = G_IO_IN;
	m_ctx->iochDataEvtId = g_io_add_watch(m_ctx->ioch, cond, readCb, this);
}

bool NamedPipe::isExistingDir(const string &dirname, bool &hasError)
{
	errno = 0;
	hasError = true;
	struct stat buf;
	if (stat(dirname.c_str(), &buf) == -1 && errno != ENOENT) {
		MLPL_ERR("Failed to stat: %s, %s\n",
		         dirname.c_str(), strerror(errno));
		return false;
	}
	if (errno == ENOENT) {
		hasError = false;
		return false;
	}

	// chekc the the path is directory
	if (!S_ISDIR(buf.st_mode)) {
		MLPL_ERR("Already exist: but not directory: %s, mode: %x\n",
		         dirname.c_str(), buf.st_mode);
		return false;
	}
	if (((buf.st_mode & 0777) & BASE_DIR_MODE) != BASE_DIR_MODE) {
		MLPL_ERR("Invalid directory mode: %s, 0%o\n",
		         dirname.c_str(), buf.st_mode);
		return false;
	}

	hasError = false;
	return true;
}

bool NamedPipe::makeBasedirIfNeeded(const string &baseDir)
{
	bool hasError = false;
	if (isExistingDir(baseDir, hasError))
		return true;
	if (hasError)
		return false;

	// make a directory
	if (mkdir(BASE_DIR, BASE_DIR_MODE) == -1) {
		if (errno != EEXIST) {
			MLPL_ERR("Failed to make dir: %s, %s\n",
			         BASE_DIR, strerror(errno));
			return false;
		}
		// The other process or thread may create the directory
		// after we checked it.
		return isExistingDir(baseDir, hasError);
	}
	return true;
}

bool NamedPipe::deleteFileIfExists(const string &path)
{
	struct stat buf;
	if (stat(path.c_str(), &buf) == -1 && errno != ENOENT) {
		MLPL_ERR("Failed to stat: %s, %s\n",
		         path.c_str(), strerror(errno));
		return false;
	}
	if (errno == ENOENT)
		return true;

	if (unlink(path.c_str()) == -1) {
		MLPL_ERR("Failed to unlink: %s, %s\n",
		         path.c_str(), strerror(errno));
		return false;
	}

	return true;
}

bool NamedPipe::checkGIOStatus(GIOStatus stat, GError *error)
{
	if (stat == G_IO_STATUS_ERROR || stat == G_IO_STATUS_EOF) {
		// The error calball back will be called.
		MLPL_ERR("GIOStatus: %d, %s\n", stat,
		         error ? error->message : "reason: unknown");
		return false;
	}
	return true;
}
