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

#include <sys/time.h>
#include <errno.h>
#include "SmartTime.h"
#include "Logger.h"

using namespace mlpl;

struct SmartTime::PrivateContext {
	double time;

	// constructor
	PrivateContext(void)
	: time(0)
	{
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
		m_ctx->time = getCurrTime();
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

	m_ctx->time = ts.tv_sec + ts.tv_nsec/1.0e9;
}

SmartTime::~SmartTime()
{
	if (m_ctx)
		delete m_ctx;
}

void SmartTime::setCurrTime(void)
{
	m_ctx->time = getCurrTime();
}

void SmartTime::setTime(double time)
{
	m_ctx->time = time;
}

double SmartTime::getAsSec(void) const
{
	return m_ctx->time;
}

double SmartTime::getAsMSec(void) const
{
	return m_ctx->time * 1.0e3;
}

double SmartTime::getCurrTime(void)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		MLPL_ERR("Failed to call gettimeofday(): errno: %d\n", errno);
		return -1;
	}
	return (tv.tv_sec + tv.tv_usec/1.0e6);
}

SmartTime &SmartTime::operator-=(const SmartTime &rhs)
{
	m_ctx->time -= rhs.m_ctx->time;
	return *this;
}

