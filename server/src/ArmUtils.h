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

#ifndef ArmUtils_h
#define ArmUtils_h

#include <Monitoring.h>
#include "DBTablesConfig.h"

class ArmUtils {
public:
	struct ArmTrigger {
		TriggerIdType     triggerId;
		TriggerStatusType status;
		std::string       msg;
	};

	static void createTrigger(const MonitoringServerInfo &serverInfo,
	                          const ArmTrigger &armTrigger,
	                          TriggerInfoList &triggerInfoList);

	static void createEvent(const MonitoringServerInfo &svInfo,
	                        const ArmTrigger &armTrigger,
	                         EventInfoList &eventInfoList);
};

#endif // ArmUtils_h


