/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#ifndef ArmZabbixAPI_h
#define ArmZabbixAPI_h

#include <libsoup/soup.h>
#include "ZabbixAPI.h"
#include "ArmBase.h"
#include "ItemTablePtr.h"
#include "JSONBuilder.h"
#include "DBTablesConfig.h"

class ArmZabbixAPI : public ZabbixAPI, public ArmBase
{
public:
	typedef ItemTablePtr
	  (ArmZabbixAPI::*DataGetter)(const std::vector<uint64_t> &idVector);
	typedef void (ArmZabbixAPI::*TableSaver)(ItemTablePtr &tablePtr);

	static const int POLLING_DISABLED = -1;
	static const int DEFAULT_SERVER_PORT = 80;

	ArmZabbixAPI(const MonitoringServerInfo &serverInfo);
	virtual ~ArmZabbixAPI();

	virtual void onGotNewEvents(const ItemTablePtr &itemPtr);

protected:
	ItemTablePtr updateTriggers(void);
	void updateItems(void);

	/**
	 * get all hosts in the ZABBIX server and save them in the Hatohol DB.
	 */
	void updateHosts(void);

	void updateEvents(void);

	/**
	 * get all applications in the ZABBIX server and save them
	 * in the replica DB.
	 */
	void updateApplications(void);

	void updateGroups(void);

	void makeHatoholTriggers(ItemTablePtr triggers);
	void makeHatoholEvents(ItemTablePtr events);
	void makeHatoholItems(ItemTablePtr items, ItemTablePtr applications);
	void makeHatoholHostgroups(ItemTablePtr groups);
	void makeHatoholMapHostsHostgroups(ItemTablePtr hostsGroups);
	void makeHatoholHosts(ItemTablePtr hosts);

	uint64_t getMaximumNumberGetEventPerOnce(void);

	// virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg);
	virtual ArmPollingResult mainThreadOneProc(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // ArmZabbixAPI_h
