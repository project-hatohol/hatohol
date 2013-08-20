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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>
using namespace std;

#include "HatoholException.h"
#include "NamedPipe.h"


const char *NamedPipe::BASE_DIR = "/tmp/hatohol";

struct NamedPipe::PrivateContext {
	int fd;
	string path;
	EndType endType;

	PrivateContext(EndType _endType)
	: endType(_endType)
	{
		fd = -1;
	}

	virtual ~PrivateContext()
	{
		if (fd >= 0)
			close(fd);
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

bool NamedPipe::open(const string &name)
{
	HATOHOL_ASSERT(m_ctx->endType == END_TYPE_MASTER_READ ||
	               m_ctx->endType == END_TYPE_MASTER_WRITE,
	               "The master only can call open(): %d.\n",
	               m_ctx->endType);
	HATOHOL_ASSERT(m_ctx->fd == -1,
	               "FD must be -1 (%d). NamedPipe::open() is possibly "
	               "called multiple times.\n", m_ctx->fd);
	if (!makeBasedirIfNeeded())
		return false;

	int suffix = (m_ctx->endType == END_TYPE_MASTER_READ) ? 0 : 1;
	m_ctx->path = StringUtils::sprintf("%s/%s-%d",
	                                   BASE_DIR, name.c_str(), suffix);
	if (!deleteFileIfExists(m_ctx->path))
		return false;
	if (mkfifo(m_ctx->path.c_str(), 0600) == -1) { 
		MLPL_ERR("Failed to make FIFO: %s, %s\n",
		         m_ctx->path.c_str(), strerror(errno));
		return false;
	}

	// TODO: open the fifo
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);

	return false;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool NamedPipe::makeBasedirIfNeeded(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}

bool NamedPipe::deleteFileIfExists(const string &path)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}
