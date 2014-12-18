/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#ifndef ThreadLocalDBCache_h
#define ThreadLocalDBCache_h

#include "DBTablesMonitoring.h"
#include "DBTablesUser.h"
#include "DBTablesConfig.h"
#include "DBTablesAction.h"
#include "DBHatohol.h"

class ThreadLocalDBCache
{
public:
	static void reset(void);

	/**
	 * Delete cache for the caller thread.
	 */
	static void cleanup(void);
	static size_t getNumberOfDBClientMaps(void);

	ThreadLocalDBCache(void);
	virtual ~ThreadLocalDBCache();

	DBHatohol       &getDBHatohol(void);

	DBTablesConfig  &getConfig(void);
	DBTablesUser    &getUser(void);
	DBTablesAction  &getAction(void);
	DBTablesMonitoring &getMonitoring(void);

private:
	/**
	 * An instance of this class is designed to be defined
	 * only as a stack variable. The following as a private makes it
	 * impossible to allocate an instance with a 'new' operator.
	 */
	static void *operator new(size_t);

	struct Impl;
};

#endif // ThreadLocalDBCache_h
