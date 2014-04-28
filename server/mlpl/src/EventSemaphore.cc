/*
 * Copyright (C) 2014 Project Hatohol
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
#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include "EventSemaphore.h"
#include "Logger.h"

using namespace mlpl;

struct EventSemaphore::PrivateContext {
	int eventFd;

	PrivateContext(const int &count)
	: eventFd(-1)
	{
		eventFd = eventfd(count, EFD_CLOEXEC|EFD_SEMAPHORE);
		if (eventFd == -1) {
			MLPL_CRIT("Failed to create event fd: errno: %d\n",
			          errno);
			abort();
		}
	}

	virtual ~PrivateContext()
	{
		if (eventFd >= 0)
			close(eventFd);
	}
};

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
EventSemaphore::EventSemaphore(const int &count)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(count);
}

EventSemaphore::~EventSemaphore()
{
	if (m_ctx)
		delete m_ctx;
}

void EventSemaphore::post(void)
{
retry:
	const uint64_t buf = 1;
	ssize_t ret = write(m_ctx->eventFd, &buf, sizeof(buf));
	if (ret == -1) {
		if (errno == EINTR)
			goto retry;
		MLPL_CRIT("Failed to write: %d\n", errno);
	}
}

uint64_t EventSemaphore::wait(void)
{
retry:
	uint64_t buf = 0;
	ssize_t ret = read(m_ctx->eventFd, &buf, sizeof(buf));
	if (ret == -1) {
		if (errno == EINTR)
			goto retry;
		MLPL_CRIT("Failed to write: %d\n", errno);
	}
	return buf;
}

int EventSemaphore::getEventFd(void) const
{
	return m_ctx->eventFd;
}

