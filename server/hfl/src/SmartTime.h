/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#ifndef SmartTime_h
#define SmartTime_h

#include <time.h>
#include <ostream>
#include <stdint.h>

namespace hfl {

struct Time {
	uint64_t tv_sec;
	uint64_t tv_nsec;
};

class SmartTime {
public:
	enum InitType {
		INIT_NONE,
		INIT_CURR_TIME,
	};
	static const long NANO_SEC_PER_SEC = 1000 * 1000 * 1000;

	static SmartTime getCurrTime(void);

	SmartTime(InitType initType = INIT_NONE);
	SmartTime(const SmartTime &stime);
	SmartTime(const timespec &ts);
	virtual ~SmartTime();

	void setCurrTime(void);
	double getAsSec(void) const;
	double getAsMSec(void) const;
	const timespec &getAsTimespec(void) const;

	/**
	 * Check if the instance has a valid time.
	 *
	 * @return
	 * If time is the default value (zero), false is turned.
	 * Otherwise, true.
	 */
	bool hasValidTime(void) const;

	SmartTime &operator+=(const timespec &rhs);
	SmartTime &operator-=(const SmartTime &rhs);
	SmartTime &operator=(const SmartTime &rhs);
	bool operator==(const SmartTime &rhs) const;
	bool operator>=(const SmartTime &rhs) const;
	bool operator>(const SmartTime &rhs) const;
	bool operator<=(const SmartTime &rhs) const;
	bool operator<(const SmartTime &rhs) const;
	operator std::string () const;

private:
	timespec m_time;
};

} // namespace hfl

std::ostream &operator<<(std::ostream &os, const hfl::SmartTime &stime);

#endif // SmartTime_h
