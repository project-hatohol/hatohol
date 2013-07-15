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

#ifndef TimeCounter_h
#define TimeCounter_h

class TimeCounter {
public:
	enum InitType {
		INIT_NONE,
		INIT_CURR_TIME,
	};

	static double getCurrTime(void);

	TimeCounter(InitType initType = INIT_NONE);
	virtual ~TimeCounter();

	void setCurrTime(void);
	void setTime(double time);
	double getAsSec(void) const;
	double getAsMSec(void) const;

	TimeCounter &operator-=(const TimeCounter &rhs);

private:
	double m_time;
};

#endif // TimeCounter_h
