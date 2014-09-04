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

#ifndef DataStoreRedmine_h
#define DataStoreRedmine_h

#include "ArmRedmine.h"
#include "DataStore.h"

class DataStoreRedmine : public DataStore {
public:
	DataStoreRedmine(const IncidentTrackerInfo &trackerInfo,
			 const bool &autoStart = true);
	virtual ~DataStoreRedmine();

	virtual ArmBase &getArmBase(void);
	void setCopyOnDemandEnable(bool enable);
private:
	ArmRedmine m_arm;
};

#endif // DataStoreRedmine_h
