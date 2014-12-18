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

#ifndef HapZabbixAPI_h
#define HapZabbixAPI_h

#include <memory>
#include <SmartTime.h>
#include "Params.h"
#include "ZabbixAPI.h"
#include "HatoholArmPluginBase.h"

class HapZabbixAPI : public HatoholArmPluginBase, public ZabbixAPI {
public:
	HapZabbixAPI(void);
	virtual ~HapZabbixAPI();

protected:
	void workOnTriggers(void);
	void workOnHostsAndHostgroupElements(void);
	void workOnHostgroups(void);
	void workOnEvents(void);

	void parseReplyGetMonitoringServerInfoOnInitiated(
	  MonitoringServerInfo &serverInfo, const mlpl::SmartBuffer &replyBuf);

	virtual void onInitiated(void) override;
	virtual void onReady(const MonitoringServerInfo &serverInfo);
	virtual void onGotNewEvents(ItemTablePtr eventsTablePtr);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // HapZabbixAPI_h
