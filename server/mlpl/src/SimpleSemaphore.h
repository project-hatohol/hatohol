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

#ifndef SimpleSemaphore_h
#define SimpleSemaphore_h

#include <cstddef>

namespace mlpl {

class SimpleSemaphore {
public:
	enum Status {
		STAT_OK,
		STAT_TIMEDOUT,
		STAT_ERROR_UNKNOWN,
	};

	/**
	 * A constructor of SimpleSemaphore.
	 *
	 * @param count An initial count of the semaphore.
	 */
	SimpleSemaphore(const int &count = 1);

	virtual ~SimpleSemaphore();

	/**
	 * Post a semaphore.
	 *
	 * @return 0 on Succeess, or errno if an error occured.
	 */
	int post(void);

	/**
	 * Wait a semaphore.
	 *
	 * @return 0 on Succeess, or errno if an error occured.
	 */
	int wait(void);

	/**
	 * Wait a semaphore with a timeout value.
	 *
	 * @param timeoutInMSec A timeout in millisecond.
	 * @return the return status.
	 */
	Status timedWait(size_t timeoutInMSec);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

} // namespace mlpl

#endif // SimpleSemaphore_h

