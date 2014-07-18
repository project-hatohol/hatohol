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

#ifndef CacheServiceDBClient_h
#define CacheServiceDBClient_h

#include "DBClientHatohol.h"
#include "DBClientUser.h"
#include "DBClientConfig.h"
#include "DBClientAction.h"

class CacheServiceDBClient
{
public:
	static void reset(void);

	/**
	 * Delete cache for the caller thread.
	 */
	static void cleanup(void);
	static size_t getNumberOfDBClientMaps(void);

	CacheServiceDBClient(void);
	virtual ~CacheServiceDBClient();
	DBClientHatohol *getHatohol(void);
	DBClientUser    *getUser(void);
	DBClientConfig  *getConfig(void);
	DBClientAction  *getAction(void);

private:
	/**
	 * An instance of this class is designed to be defined
	 * only as a stack variable. The following as a private makes it
	 * impossible to allocate an instance with a 'new' operator.
	 */
	static void *operator new(size_t) {return NULL;}

	struct PrivateContext;

	template <class T> T *get(DBDomainId domainId);
};

#endif // CacheServiceDBClient_h
