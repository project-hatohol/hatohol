/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
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

	static void post(SimpleSemaphore *sem);

	/**
	 * Wait a semaphore.
	 *
	 * @return 0 on Succeess, or errno if an error occured.
	 */
	int wait(void);

	/**
	 * Try to wait.
	 *
	 * @return 0 on Succeess, or errno if an error occured.
	 */
	int tryWait(void);

	/**
	 * Wait a semaphore with a timeout value.
	 *
	 * @param timeoutInMSec A timeout in millisecond.
	 * @return the return status.
	 */
	Status timedWait(size_t timeoutInMSec);

	/**
	 * Initialize semaphore forcely.
	 *
	 * This method must not be used while any method of the instance
	 * is being called.
	 *
	 * @param count An initial count of the semaphore.
	 */
	void init(const int &count);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

} // namespace mlpl

