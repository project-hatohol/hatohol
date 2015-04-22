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

#ifndef HatoholArmPluginGateHAPI2_h
#define HatoholArmPluginGateHAPI2_h

#include "DataStore.h"
#include <HatoholArmPluginInterface.h>

class HatoholArmPluginGateHAPI2 : public DataStore, public HatoholArmPluginInterface {
public:
  HatoholArmPluginGateHAPI2(const MonitoringServerInfo &serverInfo);

protected:
	virtual ~HatoholArmPluginGateHAPI2();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

typedef UsedCountablePtr<HatoholArmPluginGateHAPI2> HatoholArmPluginGateHAPI2Ptr;

#endif // HatoholArmPluginGateHAPI2_h
