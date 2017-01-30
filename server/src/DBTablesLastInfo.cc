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

#include <exception>
#include <SeparatorInjector.h>
#include "Utils.h"
#include "ConfigManager.h"
#include "ThreadLocalDBCache.h"
#include "DBAgentFactory.h"
#include "DBTablesMonitoring.h"
#include "DBTablesLastInfo.h"
#include "Mutex.h"
#include "ItemGroupStream.h"
#include "UnifiedDataStore.h"
#include "DBTermCStringProvider.h"
using namespace std;
using namespace mlpl;

constexpr const char *TABLE_NAME_LAST_INFO = "last_info";

const int DBTablesLastInfo::LAST_INFO_DB_VERSION =
  DBTables::Version::getPackedVer(0, 1, 0);

static void operator>>(
  ItemGroupStream &itemGroupStream, LastInfoType &lastInfoType)
{
	lastInfoType = itemGroupStream.read<int, LastInfoType>();
}

constexpr static const ColumnDef COLUMN_DEF_LAST_INFO[] = {
{
	"last_info_id",                    // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"data_type",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"value",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	32767,                             // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_LAST_INFO_ID,
	IDX_LAST_INFO_DATA_TYPE,
	IDX_LAST_INFO_VALUE,
	IDX_LAST_INFO_SERVER_ID,
	NUM_IDX_LAST_INFO,
};

constexpr static const int columnIndexesLastUniqId[] = {
  IDX_LAST_INFO_DATA_TYPE, IDX_LAST_INFO_SERVER_ID, DBAgent::IndexDef::END,
};

constexpr static const DBAgent::IndexDef indexDefsLastInfo[] = {
  {"LastUniqId", (const int *)columnIndexesLastUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileLastInfo =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_LAST_INFO,
			    COLUMN_DEF_LAST_INFO,
			    NUM_IDX_LAST_INFO,
			    indexDefsLastInfo);

static bool updateDB(
  DBAgent &dbAgent, const DBTables::Version &oldPackedVer, void *data)
{
	return true;
}

struct DBTablesLastInfo::Impl
{
	Impl(void)
	{
	}

	virtual ~Impl()
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
const char *DBTablesLastInfo::getTableNameLastInfo(void)
{
	return TABLE_NAME_LAST_INFO;
}

void DBTablesLastInfo::reset(void)
{
	getSetupInfo().initialized = false;
}

const DBTables::SetupInfo &DBTablesLastInfo::getConstSetupInfo(void)
{
	return getSetupInfo();
}

DBTablesLastInfo::DBTablesLastInfo(DBAgent &dbAgent)
: DBTables(dbAgent, getSetupInfo()),
  m_impl(new Impl())
{
}

DBTablesLastInfo::~DBTablesLastInfo()
{
}

LastInfoIdType DBTablesLastInfo::upsertLastInfo(
  LastInfoDef &lastInfoDef, const OperationPrivilege &privilege,
  const bool &useTransaction)
{
	HatoholError err = checkPrivilegeForAdd(privilege, lastInfoDef);
	if (err != HTERR_OK)
		return INVALID_LAST_INFO_ID;

	LastInfoIdType lastInfoId;
	DBAgent::InsertArg arg(tableProfileLastInfo);
	arg.add(lastInfoDef.id);
	arg.add(lastInfoDef.dataType);
	arg.add(lastInfoDef.value);
	arg.add(lastInfoDef.serverId);
	arg.upsertOnDuplicate = true;

	DBAgent &dbAgent = getDBAgent();
	if (useTransaction) {
		dbAgent.runTransaction(arg, lastInfoId);
	} else {
		dbAgent.insert(arg);
		lastInfoId = dbAgent.getLastInsertId();
	}
	return lastInfoId;
}

HatoholError DBTablesLastInfo::updateLastInfo(LastInfoDef &lastInfoDef,
                                            const OperationPrivilege &privilege)
{
	HatoholError err = checkPrivilegeForUpdate(privilege, lastInfoDef);
	if (err != HTERR_OK)
		return err;

	DBAgent::UpdateArg arg(tableProfileLastInfo);

	const char *lastInfoIdColumnName =
	  COLUMN_DEF_LAST_INFO[IDX_LAST_INFO_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%" FMT_LAST_INFO_ID,
	                                     lastInfoIdColumnName, lastInfoDef.id);
	arg.add(IDX_LAST_INFO_DATA_TYPE, lastInfoDef.dataType);
	arg.add(IDX_LAST_INFO_VALUE, lastInfoDef.value);
	arg.add(IDX_LAST_INFO_SERVER_ID, lastInfoDef.serverId);

	getDBAgent().runTransaction(arg);
	return HTERR_OK;
}

HatoholError DBTablesLastInfo::getLastInfoList(LastInfoDefList &lastInfoDefList,
					       const LastInfoQueryOption &option)
{

	DBAgent::SelectExArg arg(tableProfileLastInfo);
	arg.add(IDX_LAST_INFO_ID);
	arg.add(IDX_LAST_INFO_DATA_TYPE);
	arg.add(IDX_LAST_INFO_VALUE);
	arg.add(IDX_LAST_INFO_SERVER_ID);

	// condition
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	for (auto itemGrp : grpList) {
		ItemGroupStream itemGroupStream(itemGrp);
		LastInfoDef lastInfoDef;

		itemGroupStream >> lastInfoDef.id;
		itemGroupStream >> lastInfoDef.dataType;
		itemGroupStream >> lastInfoDef.value;
		itemGroupStream >> lastInfoDef.serverId;

		lastInfoDefList.push_back(lastInfoDef);
	}

	return HTERR_OK;
}

static string makeIdListCondition(const LastInfoIdList &idList)
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_LAST_INFO[IDX_LAST_INFO_ID];
	SeparatorInjector commaInjector(",");
	condition = StringUtils::sprintf("%s in (", colId.columnName);
	for (auto id : idList) {
		commaInjector(condition);
		condition += StringUtils::sprintf("%" FMT_LAST_INFO_ID, id);
	}

	condition += ")";
	return condition;
}

static string makeServerIdCondition(LastInfoServerIdType serverId)
{
	return StringUtils::sprintf(
		 "%s=%" FMT_LAST_INFO_SERVER_ID,
		 COLUMN_DEF_LAST_INFO[IDX_LAST_INFO_SERVER_ID].columnName,
		 serverId);
}
static string makeConditionForDelete(const LastInfoIdList &idList,
				     const OperationPrivilege &privilege)
{
	string condition = makeIdListCondition(idList);

	return condition;
}

HatoholError DBTablesLastInfo::deleteLastInfoList(const LastInfoIdList &idList,
                                                  const OperationPrivilege &privilege)
{
	HatoholError err = checkPrivilegeForDelete(privilege, idList);
	if (err != HTERR_OK)
		return err;

	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg arg;
		uint64_t numAffectedRows;

		TrxProc (void)
		: arg(tableProfileLastInfo),
		  numAffectedRows(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(arg);
			numAffectedRows = dbAgent.getNumberOfAffectedRows();
		}
	} trx;
	trx.arg.condition = makeConditionForDelete(idList, privilege);
	getDBAgent().runTransaction(trx);

	// Check the result
	if (trx.numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         trx.numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
	}

	return HTERR_OK;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
DBTables::SetupInfo &DBTablesLastInfo::getSetupInfo(void)
{
	static const TableSetupInfo DB_TABLE_INFO[] = {
	{
		&tableProfileLastInfo,
	},
	};

	static SetupInfo setupInfo = {
		DB_TABLES_ID_LAST_INFO,
		LAST_INFO_DB_VERSION,
		ARRAY_SIZE(DB_TABLE_INFO),
		DB_TABLE_INFO,
		updateDB,
	};
	return setupInfo;
};

HatoholError DBTablesLastInfo::checkPrivilegeForAdd(
  const OperationPrivilege &privilege, const LastInfoDef &actionDef)
{
	const UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	return HTERR_OK;
}

HatoholError DBTablesLastInfo::checkPrivilegeForDelete(
  const OperationPrivilege &privilege, const LastInfoIdList &idList)
{
	const UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	return HTERR_OK;
}

HatoholError DBTablesLastInfo::checkPrivilegeForUpdate(
  const OperationPrivilege &privilege, const LastInfoDef &lastInfoDef)
{
	const UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	return HTERR_OK;
}

// ---------------------------------------------------------------------------
// LastInfoQueryOption
// ---------------------------------------------------------------------------
struct LastInfoQueryOption::Impl {
	static const string conditionTemplate;

	LastInfoQueryOption *option;
	LastInfoType         type;
	LastInfoIdList       idList;
	LastInfoServerIdType serverId;

	Impl(LastInfoQueryOption *_option)
	: option(_option), type(LAST_INFO_ALL), serverId(INVALID_LAST_INFO_SERVER_ID)
	{
	}

	static string makeConditionTemplate(void);
	string getLastInfoTypeCondition(void);
};

const string LastInfoQueryOption::Impl::conditionTemplate
  = makeConditionTemplate();

string LastInfoQueryOption::Impl::makeConditionTemplate(void)
{
	string cond;

	// data_type;
	const ColumnDef &colDefDataType =
	  COLUMN_DEF_LAST_INFO[IDX_LAST_INFO_DATA_TYPE];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%d))",
	  colDefDataType.columnName, colDefDataType.columnName);
	cond += " AND ";

	// value;
	const ColumnDef &colDefValue = COLUMN_DEF_LAST_INFO[IDX_LAST_INFO_VALUE];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%s))",
	  colDefValue.columnName, colDefValue.columnName);
	cond += " AND ";

	// serverId;
	const ColumnDef &colDefServerId =
	   COLUMN_DEF_LAST_INFO[IDX_LAST_INFO_SERVER_ID];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR %s IN (%%s))",
	  colDefServerId.columnName, colDefServerId.columnName);
	cond += " AND ";

	return cond;
}

LastInfoQueryOption::LastInfoQueryOption(const UserIdType &userId)
: DataQueryOption(userId), m_impl(new Impl(this))
{
}

LastInfoQueryOption::LastInfoQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext), m_impl(new Impl(this))
{
}

LastInfoQueryOption::LastInfoQueryOption(const LastInfoQueryOption &src)
: DataQueryOption(src), m_impl(new Impl(this))
{
	*m_impl = *src.m_impl;
}

LastInfoQueryOption::~LastInfoQueryOption()
{
}

void LastInfoQueryOption::setTargetServerId(const LastInfoServerIdType serverId)
{
	m_impl->serverId = serverId;
}

const LastInfoServerIdType LastInfoQueryOption::getTargetServerId(void)
{
	return m_impl->serverId;
}

void LastInfoQueryOption::setLastInfoType(const LastInfoType &type)
{
	m_impl->type = type;
}

const LastInfoType &LastInfoQueryOption::getLastInfoType(void)
{
	return m_impl->type;
}

void LastInfoQueryOption::setLastInfoIdList(const LastInfoIdList &idList)
{
	m_impl->idList = idList;
}

const LastInfoIdList &LastInfoQueryOption::getLastInfoIdList(void)
{
	return m_impl->idList;
}

string LastInfoQueryOption::Impl::getLastInfoTypeCondition(void)
{
	string condition;
	switch(type) {
	case LAST_INFO_ALL:
		return condition;
	default:
		condition += StringUtils::sprintf("data_type=%d", (int)type);
		return condition;
	}
}

string LastInfoQueryOption::getCondition(void) const
{
	using StringUtils::sprintf;
	string cond;

	// filter by action type
	string lastInfoTypeCondition = m_impl->getLastInfoTypeCondition();
	if (!lastInfoTypeCondition.empty()) {
		if (!cond.empty())
			cond += " AND ";
		cond += lastInfoTypeCondition;
	}

	// filter by ID list
	if (!m_impl->idList.empty()) {
		if (!cond.empty())
			cond += " AND ";
		cond += makeIdListCondition(m_impl->idList);
	}

	HATOHOL_ASSERT(!m_impl->conditionTemplate.empty(),
	               "LastInfoDef condition template is empty.");

	if (m_impl->serverId != INVALID_LAST_INFO_SERVER_ID) {
		if (!cond.empty())
			cond += " AND ";
		cond += makeServerIdCondition(m_impl->serverId);
	}

	return cond;
}
