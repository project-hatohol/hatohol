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

#ifndef ArmNagiosNDOUtils_h
#define ArmNagiosNDOUtils_h

#include <libsoup/soup.h>
#include "ArmBase.h"
#include "ItemTablePtr.h"
#include "JSONParser.h"
#include "JSONBuilder.h"
#include "DBTablesConfig.h"

class ArmNagiosNDOUtils : public ArmBase
{
public:
	ArmNagiosNDOUtils(const MonitoringServerInfo &serverInfo);
	virtual ~ArmNagiosNDOUtils();

protected:
	void makeSelectTriggerBuilder(void);
	void makeSelectEventBuilder(void);
	void makeSelectItemBuilder(void);
	void makeSelectHostArg(void);
	void makeSelectHostgroupArg(void);
	void makeSelectHostgroupMembersArg(void);
	void addConditionForTriggerQuery(void);
	void addConditionForEventQuery(void);
	void getTrigger(void);
	void getEvent(void);
	void getItem(void);
	void getHost(void);
	void getHostgroup(void);
	void getHostgroupMembers(void);
	void connect(void);

	// virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg);
	virtual ArmPollingResult mainThreadOneProc(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // ArmNagiosNDOUtils_h
