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

class ReadWriteLock {
public:
	ReadWriteLock(void);
	virtual ~ReadWriteLock();
	void readLock(void);
	void writeLock(void);
	void unlock(void);

	static void unlock(ReadWriteLock *rwlock);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

} // namespace mlpl

