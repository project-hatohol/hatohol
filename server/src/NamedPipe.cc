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
using namespace mlpl;

#include "HatoholException.h"
#include "NamedPipe.h"

const char *NamedPipe::BASE_DIR = "/tmp/hatohol";
unsigned BASE_DIR_MODE =
   S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP;
unsigned FIFO_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

typedef list<SmartBuffer *>       SmartBufferList;
typedef SmartBufferList::iterator SmartBufferListIterator;

static const gint INVALID_EVENT_ID = -1;

struct NamedPipe::PrivateContext {
	int fd;
	string path;
	EndType endType;
	GIOChannel *ioch;
	gint iochEvtId;
	bool writeCbSet;
	bool writeReady;
	SmartBufferList writeBufList;

	PrivateContext(EndType _endType)
	: fd(-1),
	  endType(_endType),
	  ioch(NULL),
	  iochEvtId(INVALID_EVENT_ID),
	  writeReady(false)
	{
	}

	virtual ~PrivateContext()
	{
		if (ioch)
			g_object_unref(ioch);
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
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
NamedPipe::NamedPipe(EndType endType)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(endType);
}

NamedPipe::~NamedPipe()
{
	if (m_ctx)
		delete m_ctx;
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
		openFlag = O_RDWR;
		recreate = true;
		break;
	case END_TYPE_SLAVE_READ:
		suffix = 1;
		openFlag = O_RDONLY;
		break;
	case END_TYPE_SLAVE_WRITE:
		suffix = 0;
		openFlag = O_WRONLY;
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

bool NamedPipe::createGIOChannel
  (GIOCondition cond, GIOFunc iochCb, gpointer data)
{
	HATOHOL_ASSERT(m_ctx->fd > 0, "Invalid FD\n");
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

	if (m_ctx->endType == END_TYPE_MASTER_WRITE
	    || m_ctx->endType == END_TYPE_SLAVE_WRITE) {
		HATOHOL_ASSERT(!iochCb,
		               "iochCB cannot be specified in slave mode.\n");
		enableWriteCbIfNeeded();
		return true;
	}
	if (iochCb) {
		m_ctx->iochEvtId =
		  g_io_add_watch(m_ctx->ioch, cond, iochCb, data);
	}
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
	HATOHOL_ASSERT(m_ctx->endType == END_TYPE_MASTER_WRITE
	               || m_ctx->endType == END_TYPE_SLAVE_WRITE,
	               "push() can be called only by writers: %d\n",
	               m_ctx->endType);
	buf.resetIndex();
	if (m_ctx->writeReady) {
		gchar *dataPtr = buf.getPointer<gchar>();
		gssize count = buf.size();
		gsize bytesWritten;
		GError *error = NULL;
		GIOStatus stat =
		  g_io_channel_write_chars(m_ctx->ioch, dataPtr, count,
		                           &bytesWritten, &error);
		if (stat == G_IO_STATUS_ERROR || stat == G_IO_STATUS_EOF) {
			// TODO: add error sequence
		}
		if (stat == G_IO_STATUS_AGAIN) {
			MLPL_BUG(
			  "received G_IO_STATUS_AGAIN. However, the pipe was "
			  "opened without O_NONBLOCK. Something is wrong.");
			m_ctx->closeFd(); // This will cause the HUP event.
		}
		if (bytesWritten == (gsize)count)
			return;
		buf.incIndex(bytesWritten);
	}
	enableWriteCbIfNeeded();
	m_ctx->writeBufList.push_back(buf.takeOver());
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gboolean NamedPipe::writeCb(GIOChannel *source, GIOCondition condition,
                            gpointer data)
{
	NamedPipe *obj = static_cast<NamedPipe *>(data);
	PrivateContext *ctx = obj->m_ctx;
	ctx->writeReady = true;
	ctx->iochEvtId = INVALID_EVENT_ID;
	if (ctx->writeBufList.empty()) {
		MLPL_BUG("writeCB was called. "
		         "However, write buffer list is empty.\n");
		return FALSE;
	}
	// TODO: add code to write buffer m_ctx->writeBufList is not empty;
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);

	return FALSE;
}

void NamedPipe::enableWriteCbIfNeeded(void)
{
	if (m_ctx->iochEvtId != INVALID_EVENT_ID)
		return;
	GIOCondition cond
	  = (GIOCondition)(G_IO_OUT|G_IO_ERR|G_IO_HUP|G_IO_NVAL);
	m_ctx->iochEvtId = g_io_add_watch(m_ctx->ioch, cond, writeCb, this);
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
