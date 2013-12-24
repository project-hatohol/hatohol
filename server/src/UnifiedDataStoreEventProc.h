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

#ifndef UnifiedDataStoreEventProc_h
#define UnifiedDataStoreEventProc_h

#include "DataStoreEventProc.h"
#include "DataStore.h"

struct UnifiedDataStoreEventProc : public DataStoreEventProc
{
public:
	bool isSetCopyOnDemandEnable;
	void onAddedStoreVector(DataStore *dataStore);
};

#endif // UnifiedDataStoreEventProc_h
