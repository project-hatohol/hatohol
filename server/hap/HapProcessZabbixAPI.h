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

#include <exception>
#include <ZabbixAPI.h>
#include "HapProcessStandard.h"

class HapProcessZabbixAPI : public HapProcessStandard, public ZabbixAPI {
public:
	HapProcessZabbixAPI(int argc, char *argv[]);
	virtual ~HapProcessZabbixAPI();

protected:
	void setMonitoringServerInfo(void);
	void workOnTriggers(void);
	void workOnHostsAndHostgroupElements(void);
	void workOnHostgroups(void);
	void workOnEvents(void);

	void parseReplyGetMonitoringServerInfoOnInitiated(
	  MonitoringServerInfo &serverInfo, const mlpl::SmartBuffer &replyBuf);

	virtual HatoholError acquireData(
	  const MessagingContext &msgCtx) override;
	virtual HatoholError fetchItem(void) override;
	virtual HatoholArmPluginWatchType getHapWatchType(
	  const HatoholError &err) override;

	virtual void onGotNewEvents(ItemTablePtr eventsTablePtr);

	virtual void onInitiated(void) override;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;;
};

