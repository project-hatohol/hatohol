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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include "PipeUtils.h"
#include "Logger.h"
#include "StringUtils.h"
#include "TimeCounter.h"

using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
PipeUtils::PipeUtils(void)
: m_fd(-1)
{
}

PipeUtils::~PipeUtils()
{
	if (m_fd >= 0)
		close(m_fd);
	if (!m_path.empty())
		unlink(m_path.c_str());
}

bool PipeUtils::makeFileInTmpAndOpenForRead(void)
{
	return makeFileInTmpAndOpen(O_RDONLY|O_NONBLOCK, 0);
}

bool PipeUtils::makeFileInTmpAndOpenForWrite(void)
{
	// We use O_RDWR to avoid blocking. Using O_WRONLY|O_NONBLOCK without
	// a reader process will fail. This mode is not defined in POSIX
	// and can be used only for Linux. (see man 7 FIFO)
	return makeFileInTmpAndOpen(O_RDWR, 1);
}

const string PipeUtils::getPath(void) const
{
	return m_path;
}

bool PipeUtils::openForRead(const string &path)
{
	m_path = path;
	return openPipe(O_RDONLY);
}

bool PipeUtils::openForWrite(const string &path)
{
	m_path = path;
	return openPipe(O_WRONLY);
}

bool PipeUtils::send(size_t size, void *data)
{
	if (write(m_fd, data, size) == -1) {
		MLPL_ERR("Failed to write: size: %zd, errno: %d\n",
		         size, errno);
		return false;
	}
	return true;
}

bool PipeUtils::recv(size_t size, void *buf, size_t timeout)
{
	size_t index = 0;
	TimeCounter startTime(TimeCounter::INIT_CURR_TIME);
	while (true) {
		if (timeout > 0) {
			TimeCounter elapsed(TimeCounter::INIT_CURR_TIME);
			elapsed -= startTime;
			timeout -= elapsed.getAsMSec();
			if (timeout <= 0) {
				MLPL_ERR("readWithTimeout: timeouted\n");
				return false;
			}
		}
		if (!waitForRead(timeout))
			return false;

		if (!readWithErrorCheck(size, (uint8_t *)buf, index))
			return false;
		if (index >= size)
			break;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
string PipeUtils::makePipeFileInTmpDir(int id)
{
	string tmpPipeName
	  = StringUtils::sprintf("/tmp/hatohol-test-pipeinfo-%d-%d",
	                         getpid(), id);
	if (mkfifo(tmpPipeName.c_str(), 0666) == -1) {
		MLPL_ERR("Failed to make FIFO: %s, errno: %d\n",
		         tmpPipeName.c_str(), errno);
		return "";
	}
	return tmpPipeName;
}

bool PipeUtils::makeFileInTmpAndOpen(int flags, int id)
{
	m_path = makePipeFileInTmpDir(id);
	if (m_path.empty())
		return false;
	return openPipe(flags);
	return true;
}

bool PipeUtils::openPipe(int flags)
{
	if (m_path.empty()) {
		MLPL_ERR("m_path: empty\n");
		return false;
	}

	m_fd = open(m_path.c_str(), flags);
	if (m_fd == -1) {
		MLPL_ERR("Failed to open: path: %s, flags: %x, errno: %d\n",
		         m_path.c_str(), flags, errno);
		return false;
	}
	return true;
}

bool PipeUtils::waitForRead(size_t timeout)
{
	pollfd fds;
	fds.fd = m_fd;
	fds.events = POLLIN;
	int pollTimeout = (timeout == 0) ? -1 : timeout;
	int ret;
	while (true) {
		ret = poll(&fds, 1, pollTimeout);
		if (ret == -1 && errno == EINTR)
			continue;
		break;
	}
	if (ret == -1) {
		MLPL_ERR("Failed to poll, errno: %d\n", errno);
		return false;
	}
	if (!(fds.revents & POLLIN)) {
		MLPL_ERR("Timeouted: poll\n");
		return false;
	}
	return true;
}

bool PipeUtils::readWithErrorCheck(size_t size, uint8_t *buf, size_t &index)
{
	while (true) {
		size_t requestSize = size - index;
		ssize_t readSize = read(m_fd, &buf[index], requestSize);
		if (readSize == 0) {
			MLPL_INFO("read size: 0\n");
			return false;
		} else if (readSize == -1 && errno == EINTR) {
			continue;
		} else if (readSize == -1) {
			MLPL_ERR("Failed to read: size: %zd, errno: %d\n",
			         size, errno);
			return false;
		}
		index += readSize;
		break;
	}
	return true;
}
