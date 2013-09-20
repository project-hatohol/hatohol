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
#include "TimeCounter.h"
#include "Logger.h"

using namespace mlpl;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
TimeCounter::TimeCounter(InitType initType)
: m_time(0)
{
	switch(initType) {
	case INIT_NONE:
		// nothing to do;
		break;
	case INIT_CURR_TIME:
		m_time = getCurrTime();
		break;
	default:
		MLPL_ERR("Unknown initType: %d\n", initType);
		break;
	}
}

TimeCounter::TimeCounter(const timespec &ts)
: m_time(0)
{
	m_time = ts.tv_sec + ts.tv_nsec/1.0e9;
}

TimeCounter::~TimeCounter()
{
}

void TimeCounter::setCurrTime(void)
{
	m_time = getCurrTime();
}

void TimeCounter::setTime(double time)
{
	m_time = time;
}

double TimeCounter::getAsSec(void) const
{
	return m_time;
}

double TimeCounter::getAsMSec(void) const
{
	return m_time * 1.0e3;
}

double TimeCounter::getCurrTime(void)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		MLPL_ERR("Failed to call gettimeofday(): errno: %d\n", errno);
		return -1;
	}
	return (tv.tv_sec + tv.tv_usec/1.0e6);
}

TimeCounter &TimeCounter::operator-=(const TimeCounter &rhs)
{
	m_time -= rhs.m_time;
	return *this;
}

