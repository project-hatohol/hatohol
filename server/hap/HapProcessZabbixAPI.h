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
	  const MessagingContext &msgCtx,
	  const mlpl::SmartBuffer &cmdBuf) override;
	virtual HatoholError fetchItem(
	  const MessagingContext &msgCtx,
	  const mlpl::SmartBuffer &cmdBuf) override;
	virtual HatoholError fetchHistory(
	  const MessagingContext &msgCtx,
	  const mlpl::SmartBuffer &cmdBuf) override;
	virtual HatoholArmPluginWatchType getHapWatchType(
	  const HatoholError &err) override;
	virtual HatoholError fetchTrigger(
	  const MessagingContext &msgCtx,
	  const mlpl::SmartBuffer &cmdBuf) override;

	virtual void onGotNewEvents(ItemTablePtr eventsTablePtr);

	virtual void onInitiated(void) override;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;;
};
