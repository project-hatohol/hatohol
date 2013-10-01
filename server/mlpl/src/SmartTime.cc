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

#include <cmath>
#include <sys/time.h>
#include <errno.h>
#include "SmartTime.h"
#include "Logger.h"

using namespace mlpl;

struct SmartTime::PrivateContext {
	timespec time;

	// constructor
	PrivateContext(void)
	{
		time.tv_sec = 0;
		time.tv_nsec = 0;
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SmartTime::SmartTime(InitType initType)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();

	switch(initType) {
	case INIT_NONE:
		// nothing to do;
		break;
	case INIT_CURR_TIME:
		*this = getCurrTime();
		break;
	default:
		MLPL_ERR("Unknown initType: %d\n", initType);
		break;
	}
}

SmartTime::SmartTime(const timespec &ts)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	m_ctx->time.tv_sec  = ts.tv_sec;
	m_ctx->time.tv_nsec = ts.tv_nsec;
}

SmartTime::~SmartTime()
{
	if (m_ctx)
		delete m_ctx;
}

void SmartTime::setCurrTime(void)
{
	*this = getCurrTime();
}

void SmartTime::setTime(double time)
{
	double integer;
	double frac = modf(time, &integer);
	m_ctx->time.tv_sec = static_cast<time_t>(integer);
	m_ctx->time.tv_nsec = (frac * 1e9);
}

double SmartTime::getAsSec(void) const
{
	return m_ctx->time.tv_sec + m_ctx->time.tv_nsec/1e9;
}

double SmartTime::getAsMSec(void) const
{
	return 1e3 * getAsSec();
}

const timespec &SmartTime::getAsTimespec(void) const
{
	return m_ctx->time;
}

SmartTime SmartTime::getCurrTime(void)
{
	SmartTime smtime;
	if (clock_gettime(CLOCK_REALTIME, &smtime.m_ctx->time) == -1) {
		MLPL_ERR("Failed to call clock_gettime(%d): errno: %d\n",
		         CLOCK_REALTIME,  errno);
	}
	return smtime;
}

SmartTime &SmartTime::operator-=(const SmartTime &rhs)
{
	m_ctx->time.tv_sec  -= rhs.m_ctx->time.tv_sec;
	m_ctx->time.tv_nsec -= rhs.m_ctx->time.tv_nsec;
	if (m_ctx->time.tv_nsec < 0) {
		m_ctx->time.tv_sec -= 1;
		m_ctx->time.tv_nsec += 1e9;
	}
	return *this;
}

SmartTime &SmartTime::operator=(const SmartTime &rhs)
{
	m_ctx->time.tv_sec  = rhs.m_ctx->time.tv_sec;
	m_ctx->time.tv_nsec = rhs.m_ctx->time.tv_nsec;
	return *this;
}
