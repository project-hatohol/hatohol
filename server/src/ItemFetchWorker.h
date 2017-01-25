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

#ifndef ItemFetchWorker_h
#define ItemFetchWorker_h

#include <deque>
#include <memory>
#include "Params.h"
#include "Closure.h"
#include "DataStore.h"
#include "DBTablesMonitoring.h"

class ItemFetchWorker
{
public:
	ItemFetchWorker(void);
	virtual ~ItemFetchWorker();

	bool start(const ItemsQueryOption &option,
	           Closure0 *closure = NULL);
	bool updateIsNeeded(void);
	void waitCompletion(void);

protected:
	void updatedCallback(Closure0 *closure);
	bool runFetcher(const LocalHostIdVector hostIds,
	                std::shared_ptr<DataStore> dataStore);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // ItemFetchWorker_h
