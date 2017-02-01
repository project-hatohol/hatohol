/*
 * Copyright (C) 2013 Project Hatohol
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
#include <pthread.h>

namespace mlpl {

class Mutex {
public:
	enum Status {
		STAT_OK,
		STAT_TIMEDOUT,
		STAT_ERROR_UNKNOWN,
	};

	Mutex(void);
	virtual ~Mutex();
	void lock(void);
	Status timedlock(size_t timeoutInMSec);
	void unlock(void);

	/**
	 * @return
	 * true on successfully locked. When the lock is already taken,
	 *  false is returned immediately.
	 */
	bool trylock(void);

	static void unlock(Mutex *lock);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

class AutoMutex {
public:
	AutoMutex(Mutex *mutex);
	virtual ~AutoMutex();
private:
	Mutex *m_mutex;
};

} // namespace mlpl

