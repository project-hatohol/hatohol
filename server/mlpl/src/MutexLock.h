/*
 * Copyright (C) 2013 Hatohol Project
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

#ifndef MutexLock_h
#define MutexLock_h

#include <pthread.h>

namespace mlpl {

class MutexLock {
public:
	MutexLock(void);
	virtual ~MutexLock();
	void lock(void);
	void unlock(void);

	/**
	 * @return
	 * true on successfully locked. When the lock is already taken,
	 *  false is returned immediately.
	 */
	bool trylock(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

} // namespace mlpl

#endif // MutexLock_h
