/*
 * Copyright (C) 2015 Project Hatohol
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

#ifndef TriggerFetchWorker_h
#define TriggerFetchWorker_h

#include "Params.h"
#include "Closure.h"
#include "DataStore.h"

class TriggerFetchWorker
{
public:
	TriggerFetchWorker(void);
	virtual ~TriggerFetchWorker();

	bool start(const ServerIdType &targetServerId = ALL_SERVERS,
	           Closure2 *closure = NULL);
	bool updateIsNeeded(void);
	void waitCompletion(void);

protected:
	void updatedCallback(Closure2 *closure);
	bool runFetcher(DataStore *dataStore);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // TriggerFetchWorker_h
