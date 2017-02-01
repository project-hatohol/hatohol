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

#pragma once
#include <string>
#include "DBTablesMonitoring.h"
#include "DBTables.h"
#include "Params.h"

constexpr const static LastInfoIdType INVALID_LAST_INFO_ID = -1;
constexpr const static LastInfoServerIdType INVALID_LAST_INFO_SERVER_ID = -1;

enum LastInfoType {
	LAST_INFO_ALL = -1,
	LAST_INFO_HOST,
	LAST_INFO_HOST_GROUP,
	LAST_INFO_HOST_GROUP_MEMBERSHIP,
	LAST_INFO_TRIGGER,
	LAST_INFO_EVENT,
	LAST_INFO_HOST_PARENT,
	NUM_LAST_INFO_TYPES,
};

struct LastInfoCondition {
	LastInfoType  dataType;
	std::string   value;
	std::string   serverId;

	// methods
	LastInfoCondition(void)
	{
	}

	LastInfoCondition(const LastInfoType &_dataType,
	                  const std::string &_value,
	                  const std::string &_serverId)
	: dataType(_dataType),
	  value(_value),
	  serverId(_serverId)
	{
	}
};

struct LastInfoDef {
	LastInfoIdType       id;
	LastInfoType         dataType;
	std::string          value;
	LastInfoServerIdType serverId;
};

typedef std::list<LastInfoDef>          LastInfoDefList;
typedef LastInfoDefList::iterator       LastInfoDefListIterator;
typedef LastInfoDefList::const_iterator LastInfoDefListConstIterator;

typedef std::list<LastInfoIdType>       LastInfoIdList;
typedef LastInfoIdList::iterator        LastInfoIdListIterator;
typedef LastInfoIdList::const_iterator  LastInfoIdListConstIterator;

class LastInfoQueryOption : public DataQueryOption {
public:
	LastInfoQueryOption(const UserIdType &userId = INVALID_USER_ID);
	LastInfoQueryOption(DataQueryContext *dataQueryContext);
	LastInfoQueryOption(const LastInfoQueryOption &src);
	~LastInfoQueryOption();

	void setTargetServerId(const LastInfoServerIdType serverId);
	const LastInfoServerIdType getTargetServerId(void);
	void setLastInfoType(const LastInfoType &type);
	const LastInfoType &getLastInfoType(void);
	void setLastInfoIdList(const LastInfoIdList &idList);
	const LastInfoIdList &getLastInfoIdList(void);

	virtual std::string getCondition(void) const override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class DBTablesLastInfo : public DBTables {

public:
	static const int LAST_INFO_DB_VERSION;

	static void init(void);
	static void reset(void);
	static const SetupInfo &getConstSetupInfo(void);
	static void stop(void);
	static const char *getTableNameLastInfo(void);

	DBTablesLastInfo(DBAgent &dbAgent);
	virtual ~DBTablesLastInfo();
	LastInfoIdType upsertLastInfo(LastInfoDef &lastInfoDef,
	                              const OperationPrivilege &privilege,
	                              const bool &useTransaction = true);
	HatoholError getLastInfoList(LastInfoDefList &lastInfoDefList,
	                             const LastInfoQueryOption &option);
	HatoholError deleteLastInfoList(const LastInfoIdList &idList,
	                                const OperationPrivilege &privilege);
	HatoholError updateLastInfo(LastInfoDef &lastInfoDef,
	                            const OperationPrivilege &privilege);

protected:
	static SetupInfo &getSetupInfo(void);

	HatoholError checkPrivilegeForAdd(
	  const OperationPrivilege &privilege, const LastInfoDef &lastInfoDef);
	HatoholError checkPrivilegeForDelete(
	  const OperationPrivilege &privilege, const LastInfoIdList &idList);
	HatoholError checkPrivilegeForUpdate(
	  const OperationPrivilege &privilege, const LastInfoDef &lastInfoDef);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

};

