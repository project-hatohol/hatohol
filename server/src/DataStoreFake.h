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

#ifndef DataStoreFake_h
#define DataStoreFake_h

#include "ArmFake.h"
#include "DataStore.h"

class DataStoreFake : public DataStore {
public:
	DataStoreFake(const MonitoringServerInfo &serverInfo,
	              const bool &autoStart = true);
	virtual ~DataStoreFake();

	virtual ArmBase &getArmBase(void); // override
	virtual void setCopyOnDemandEnable(bool enable); // override
private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DataStoreFake_h
