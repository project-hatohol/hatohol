/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <memory>

#include <MutexLock.h>
#include "DBAgentFactory.h"
#include "DBClientConfig.h"
#include "CacheServiceDBClient.h"
#include "HatoholError.h"
#include "Params.h"
#include "ItemGroupStream.h"
using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_SYSTEM  = "system";
static const char *TABLE_NAME_SERVERS = "servers";

int DBClientConfig::CONFIG_DB_VERSION = 8;
const char *DBClientConfig::DEFAULT_DB_NAME = "hatohol";
const char *DBClientConfig::DEFAULT_USER_NAME = "hatohol";
const char *DBClientConfig::DEFAULT_PASSWORD  = "hatohol";

const ServerIdSet EMPTY_SERVER_ID_SET;

static void operator>>(
  ItemGroupStream &itemGroupStream, MonitoringSystemType &monSysType)
{
	monSysType = itemGroupStream.read<int, MonitoringSystemType>();
}

static const ColumnDef COLUMN_DEF_SYSTEM[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"database_dir",                    // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"enable_face_mysql",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"face_rest_port",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"enable_copy_on_demand",           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"1",                               // defaultValue
},
};

enum {
	IDX_SYSTEM_DATABASE_DIR,
	IDX_SYSTEM_ENABLE_FACE_MYSQL,
	IDX_SYSTEM_FACE_REST_PORT,
	IDX_SYSTEM_ENABLE_COPY_ON_DEMAND,
	NUM_IDX_SYSTEM,
};

static const DBAgent::TableProfile tableProfileSystem(
  TABLE_NAME_SYSTEM, COLUMN_DEF_SYSTEM,
  sizeof(COLUMN_DEF_SYSTEM), NUM_IDX_SYSTEM);

static const ColumnDef COLUMN_DEF_SERVERS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"type",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"hostname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"ip_address",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"nickname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"port",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"polling_interval_sec",            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"retry_interval_sec",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"user_name",                       // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"password",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"db_name",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_SERVERS_ID,
	IDX_SERVERS_TYPE,
	IDX_SERVERS_HOSTNAME,
	IDX_SERVERS_IP_ADDRESS,
	IDX_SERVERS_NICKNAME,
	IDX_SERVERS_PORT,
	IDX_SERVERS_POLLING_INTERVAL_SEC,
	IDX_SERVERS_RETRY_INTERVAL_SEC,
	IDX_SERVERS_USER_NAME,
	IDX_SERVERS_PASSWORD,
	IDX_SERVERS_DB_NAME,
	NUM_IDX_SERVERS,
};

static const DBAgent::TableProfile tableProfileServers(
  TABLE_NAME_SERVERS, COLUMN_DEF_SERVERS,
  sizeof(COLUMN_DEF_SERVERS), NUM_IDX_SERVERS);

static bool validIPv4Address(const string &ipAddress);
static bool validIPv6Address(const string &ipAddress);

string MonitoringServerInfo::getHostAddress(bool forURI) const
{
	if (ipAddress.empty())
		return hostName;

	if (!forURI)
		return ipAddress;

	if (!validIPv4Address(ipAddress) && validIPv6Address(ipAddress))
		return StringUtils::sprintf("[%s]", ipAddress.c_str());
	else
		return ipAddress;
}

struct DBClientConfig::PrivateContext
{
	static DBConnectInfo connInfo;

	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}
};
DBConnectInfo DBClientConfig::PrivateContext::connInfo;

static bool updateDB(DBAgent *dbAgent, int oldVer, void *data)
{
	if (oldVer <= 5) {
		DBAgent::AddColumnsArg addColumnsArg(tableProfileSystem);
		addColumnsArg.columnIndexes.push_back(
		  IDX_SYSTEM_ENABLE_COPY_ON_DEMAND);
		dbAgent->addColumns(addColumnsArg);
	}
	if (oldVer <= 7) {
		// enable copy-on-demand by default
		DBAgent::UpdateArg arg(tableProfileSystem);
		arg.add(IDX_SYSTEM_ENABLE_COPY_ON_DEMAND, 1);
		dbAgent->update(arg);
	}
	return true;
}

// ---------------------------------------------------------------------------
// ServerQueryOption
// ---------------------------------------------------------------------------
struct ServerQueryOption::PrivateContext {
	ServerIdType targetServerId;

	PrivateContext(void)
	: targetServerId(ALL_SERVERS)
	{
	}
};

ServerQueryOption::ServerQueryOption(UserIdType userId)
: DataQueryOption(userId), m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ServerQueryOption::~ServerQueryOption()
{
	if (m_ctx)
		delete m_ctx;
}

static string serverIdCondition(const ServerIdType &serverId)
{
	const char *columnName = COLUMN_DEF_SERVERS[IDX_SERVERS_ID].columnName;
	return StringUtils::sprintf("%s=%"FMT_SERVER_ID, columnName, serverId);
}

bool ServerQueryOption::hasPrivilegeCondition(string &condition) const
{
	UserIdType userId = getUserId();

	if (userId == USER_ID_SYSTEM || has(OPPRVLG_GET_ALL_SERVER)) {
		if (m_ctx->targetServerId != ALL_SERVERS)
			condition = serverIdCondition(m_ctx->targetServerId);
		return true;
	}

	if (userId == INVALID_USER_ID) {
		MLPL_DBG("INVALID_USER_ID\n");
		condition = DBClientHatohol::getAlwaysFalseCondition();
		return true;
	}

	return false;
}

void ServerQueryOption::setTargetServerId(const ServerIdType &serverId)
{
	m_ctx->targetServerId = serverId;
}

string ServerQueryOption::getCondition(void) const
{
	string condition;
	ServerIdType targetId = m_ctx->targetServerId;

	if (hasPrivilegeCondition(condition))
		return condition;

	// check allowed servers
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	ServerHostGrpSetMap srvHostGrpSetMap;
	dbUser->getServerHostGrpSetMap(srvHostGrpSetMap, getUserId());

	size_t numServers = srvHostGrpSetMap.size();
	if (numServers == 0) {
		MLPL_DBG("No allowed server\n");
		return DBClientHatohol::getAlwaysFalseCondition();
	}

	if (targetId != ALL_SERVERS &&
	    srvHostGrpSetMap.find(targetId) != srvHostGrpSetMap.end())
	{
		return serverIdCondition(targetId);
	}

	numServers = 0;
	ServerHostGrpSetMapConstIterator it = srvHostGrpSetMap.begin();
	for (; it != srvHostGrpSetMap.end(); ++it) {
		const ServerIdType &serverId = it->first;

		if (serverId == ALL_SERVERS)
			return "";

		if (!condition.empty())
			condition += " OR ";
		condition += serverIdCondition(serverId);
		++numServers;
	}

	if (numServers == 1)
		return condition;
	return StringUtils::sprintf("(%s)", condition.c_str());
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientConfig::init(const CommandLineArg &cmdArg)
{
	//
	// set database info
	//
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		&tableProfileSystem,
		tableInitializerSystem,
	}, {
		&tableProfileServers,
	}
	};
	static const size_t NUM_TABLE_INFO =
	  sizeof(DB_TABLE_INFO) / sizeof(DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		CONFIG_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
		&updateDB,
	};

	registerSetupInfo(
	  DB_DOMAIN_ID_CONFIG, DEFAULT_DB_NAME, &DB_SETUP_FUNC_ARG);

	if (!parseCommandLineArgument(cmdArg))
		THROW_HATOHOL_EXCEPTION("Failed to parse argument.");
}

void DBClientConfig::reset(void)
{
	DBConnectInfo &connInfo = PrivateContext::connInfo;
	connInfo.user = DEFAULT_USER_NAME;
	connInfo.password = DEFAULT_PASSWORD;
	connInfo.dbName = DEFAULT_DB_NAME;

	string portStr;
	if (connInfo.port == 0)
		portStr = "(default)";
	else
		portStr = StringUtils::sprintf("%zd", connInfo.port);
	bool usePassword = !connInfo.password.empty();
	MLPL_INFO("Configuration DB Server: %s, port: %s, "
	          "DB: %s, User: %s, use password: %s\n",
	          connInfo.host.c_str(), portStr.c_str(),
	          connInfo.dbName.c_str(),
	          connInfo.user.c_str(), usePassword ? "yes" : "no");
	setConnectInfo(DB_DOMAIN_ID_CONFIG, connInfo);
}

DBClientConfig::DBClientConfig(void)
: DBClient(DB_DOMAIN_ID_CONFIG),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClientConfig::~DBClientConfig()
{
	if (m_ctx)
		delete m_ctx;
}

string DBClientConfig::getDatabaseDir(void)
{
	DBAgent::SelectArg arg(tableProfileSystem);
	arg.columnIndexes.push_back(IDX_SYSTEM_DATABASE_DIR);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<string>();
}

void DBClientConfig::setDatabaseDir(const string &dir)
{
	DBAgent::UpdateArg arg(tableProfileSystem);
	arg.add(IDX_SYSTEM_DATABASE_DIR, dir);
	DBCLIENT_TRANSACTION_BEGIN() {
		update(arg);
	} DBCLIENT_TRANSACTION_END();
}

bool DBClientConfig::isFaceMySQLEnabled(void)
{
	DBAgent::SelectArg arg(tableProfileSystem);
	arg.columnIndexes.push_back(IDX_SYSTEM_ENABLE_FACE_MYSQL);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

int  DBClientConfig::getFaceRestPort(void)
{
	DBAgent::SelectArg arg(tableProfileSystem);
	arg.columnIndexes.push_back(IDX_SYSTEM_FACE_REST_PORT);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

void DBClientConfig::setFaceRestPort(int port)
{
	DBAgent::UpdateArg arg(tableProfileSystem);
	arg.add(IDX_SYSTEM_FACE_REST_PORT, port);
	DBCLIENT_TRANSACTION_BEGIN() {
		update(arg);
	} DBCLIENT_TRANSACTION_END();
}

bool DBClientConfig::isCopyOnDemandEnabled(void)
{
	DBAgent::SelectArg arg(tableProfileSystem);
	arg.columnIndexes.push_back(IDX_SYSTEM_ENABLE_COPY_ON_DEMAND);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

bool isNumber(const string &str, bool isHex = false)
{
	for (size_t i = 0; i < str.size(); i++) {
		int ch = str[i];
		if (ch >= '0' && ch <= '9')
			continue;
		if (!isHex)
			return false;
		if (ch >= 'A' && ch <= 'F')
			continue;
		if (ch >= 'a' && ch <= 'f')
			continue;
		return false;
	}
	return true;
}

bool validIPv4Address(const string &ipAddress) {
	StringVector fields;
	StringUtils::split(fields, ipAddress,  '.');
	if (fields.size() != 4)
		return false;
	for (size_t i = 0; i < fields.size(); i++) {
		if (fields[i].empty())
			return false;
		if (!isNumber(fields[i]))
			return false;
		int value = atoi(fields[i].c_str());
		if (value < 0 || value > 255)
			return false;
	}
	return true;
}

bool validIPv6AddressField(const string &field)
{
	bool isHex = true;
	if (!isNumber(field, isHex))
		return false;
	long int value = strtol(field.c_str(), NULL, 16);
	if (value < 0 || value > 0xFFFF)
		return false;
	return true;
}

bool validIPv6Address(const string &ipAddress)
{
	if (ipAddress == "::1")
		return true;

	StringVector fields;
	StringUtils::split(fields, ipAddress,  ':');
	if (fields.size() < 3 || fields.size() > 8)
		return false;
	bool hasEmptyField = false;
	for (size_t i = 0; i < fields.size(); i++) {
		if (fields[i].empty()) {
			// only one empty filed is allowed
			if (hasEmptyField)
				return false;
			hasEmptyField = true;
			continue;
		}
		if (validIPv6AddressField(fields[i]))
			continue;
		if (i < 8 && i == fields.size() - 1 &&
		    validIPv4Address(fields[i])) {
			// It's v4 compatible address.
			// The last field is the IPv4 address.
			return true;
		} else {
			return false;
		}
	}
	return true;
}

bool validIPAddress(const string &ipAddress)
{
	if (validIPv4Address(ipAddress))
		return true;
	if (validIPv6Address(ipAddress))
		return true;
	return false;
}

bool validHostName(const string &hostName)
{
	// Currently do nothing to pass through IDN (Internationalized Domain
	// Name) to libsoup.
	return true;
}

HatoholError validServerInfo(const MonitoringServerInfo &serverInfo)
{
	if (serverInfo.type < 0 || serverInfo.type >= NUM_MONITORING_SYSTEMS)
		return HTERR_INVALID_MONITORING_SYSTEM_TYPE;
	if (serverInfo.port < 0 || serverInfo.port > 65535)
		return HTERR_INVALID_PORT_NUMBER;
	if (serverInfo.ipAddress.empty() && serverInfo.hostName.empty())
		return HTERR_NO_IP_ADDRESS_AND_HOST_NAME;
	if (!serverInfo.hostName.empty() &&
	    !validHostName(serverInfo.hostName)) {
		return HTERR_INVALID_HOST_NAME;
	}
	if (!serverInfo.ipAddress.empty() &&
	    !validIPAddress(serverInfo.ipAddress)) {
		return HTERR_INVALID_IP_ADDRESS;
	}
	return HTERR_OK;
}

HatoholError DBClientConfig::addTargetServer(
  MonitoringServerInfo *monitoringServerInfo,
  const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_CREATE_SERVER))
		return HatoholError(HTERR_NO_PRIVILEGE);

	HatoholError err = validServerInfo(*monitoringServerInfo);
	if (err != HTERR_OK)
		return err;

	DBAgent::InsertArg arg(tableProfileServers);
	arg.add(AUTO_INCREMENT_VALUE);
	arg.add(monitoringServerInfo->type);
	arg.add(monitoringServerInfo->hostName);
	arg.add(monitoringServerInfo->ipAddress);
	arg.add(monitoringServerInfo->nickname);
	arg.add(monitoringServerInfo->port);
	arg.add(monitoringServerInfo->pollingIntervalSec);
	arg.add(monitoringServerInfo->retryIntervalSec);
	arg.add(monitoringServerInfo->userName);
	arg.add(monitoringServerInfo->password);
	arg.add(monitoringServerInfo->dbName);

	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		monitoringServerInfo->id = getLastInsertId();
		// TODO: Add AccessInfo for the server to enable the operator to
		// access to it
	} DBCLIENT_TRANSACTION_END();
	return HTERR_OK;
}

HatoholError DBClientConfig::updateTargetServer(
  MonitoringServerInfo *monitoringServerInfo,
  const OperationPrivilege &privilege)
{
	if (!canUpdateTargetServer(monitoringServerInfo, privilege))
		return HatoholError(HTERR_NO_PRIVILEGE);

	HatoholError err = validServerInfo(*monitoringServerInfo);
	if (err != HTERR_OK)
		return err;

	DBAgent::UpdateArg arg(tableProfileServers);
	arg.add(IDX_SERVERS_TYPE,       monitoringServerInfo->type);
	arg.add(IDX_SERVERS_HOSTNAME,   monitoringServerInfo->hostName);
	arg.add(IDX_SERVERS_IP_ADDRESS, monitoringServerInfo->ipAddress);
	arg.add(IDX_SERVERS_NICKNAME,   monitoringServerInfo->nickname);
	arg.add(IDX_SERVERS_PORT,       monitoringServerInfo->port);
	arg.add(IDX_SERVERS_POLLING_INTERVAL_SEC,
	        monitoringServerInfo->pollingIntervalSec);
	arg.add(IDX_SERVERS_RETRY_INTERVAL_SEC,
	        monitoringServerInfo->retryIntervalSec);
	arg.add(IDX_SERVERS_USER_NAME,  monitoringServerInfo->userName);
	arg.add(IDX_SERVERS_PASSWORD,   monitoringServerInfo->password);
	arg.add(IDX_SERVERS_DB_NAME,    monitoringServerInfo->dbName);
	arg.condition = StringUtils::sprintf("id=%u", monitoringServerInfo->id);

	DBCLIENT_TRANSACTION_BEGIN() {
		if (!isRecordExisting(TABLE_NAME_SERVERS, arg.condition)) {
			err = HTERR_NOT_FOUND_TARGET_RECORD;
		} else {
			update(arg);
			err = HTERR_OK;
		}
	} DBCLIENT_TRANSACTION_END();
	return err;
}

HatoholError DBClientConfig::deleteTargetServer(
  const ServerIdType &serverId,
  const OperationPrivilege &privilege)
{
	if (!canDeleteTargetServer(serverId, privilege))
		return HatoholError(HTERR_NO_PRIVILEGE);

	DBAgent::DeleteArg arg(tableProfileServers);
	const ColumnDef &colId = COLUMN_DEF_SERVERS[IDX_SERVERS_ID];
	arg.condition = StringUtils::sprintf("%s=%"FMT_SERVER_ID,
	                                     colId.columnName, serverId);
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(arg);
	} DBCLIENT_TRANSACTION_END();
	return HTERR_OK;
}

void DBClientConfig::getTargetServers
  (MonitoringServerInfoList &monitoringServers, ServerQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileServers);
	arg.add(IDX_SERVERS_ID);
	arg.add(IDX_SERVERS_TYPE);
	arg.add(IDX_SERVERS_HOSTNAME);
	arg.add(IDX_SERVERS_IP_ADDRESS);
	arg.add(IDX_SERVERS_NICKNAME);
	arg.add(IDX_SERVERS_PORT);
	arg.add(IDX_SERVERS_POLLING_INTERVAL_SEC);
	arg.add(IDX_SERVERS_RETRY_INTERVAL_SEC);
	arg.add(IDX_SERVERS_USER_NAME);
	arg.add(IDX_SERVERS_PASSWORD);
	arg.add(IDX_SERVERS_DB_NAME);
	arg.condition = option.getCondition();

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		monitoringServers.push_back(MonitoringServerInfo());
		MonitoringServerInfo &svInfo = monitoringServers.back();

		itemGroupStream >> svInfo.id;
		itemGroupStream >> svInfo.type;
		itemGroupStream >> svInfo.hostName;
		itemGroupStream >> svInfo.ipAddress;
		itemGroupStream >> svInfo.nickname;
		itemGroupStream >> svInfo.port;
		itemGroupStream >> svInfo.pollingIntervalSec;
		itemGroupStream >> svInfo.retryIntervalSec;
		itemGroupStream >> svInfo.userName;
		itemGroupStream >> svInfo.password;
		itemGroupStream >> svInfo.dbName;
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool DBClientConfig::parseCommandLineArgument(const CommandLineArg &cmdArg)
{
	DBConnectInfo &connInfo = PrivateContext::connInfo;
	connInfo.reset();
	string dbServer;
	for (size_t i = 0; i < cmdArg.size(); i++) {
		const string &arg = cmdArg[i];
		if (arg == "--config-db-server") {
			if (i == cmdArg.size()-1) {
				MLPL_ERR(
				  "--config-db-server needs an argument.\n");
				return false;
			}
			i++;
			dbServer = cmdArg[i];
		}
	}

	if (!dbServer.empty()) {
		if (!parseDBServer(dbServer, connInfo.host, connInfo.port))
			return false;
	}

	return true;
}

void DBClientConfig::tableInitializerSystem(DBAgent *dbAgent, void *data)
{
	const ColumnDef &columnDefDatabaseDir =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_DATABASE_DIR];
	const ColumnDef &columnDefFaceRestPort =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_FACE_REST_PORT];
	const ColumnDef &columnDefEnableCopyOnDemand =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_ENABLE_COPY_ON_DEMAND];

	DBAgent::InsertArg arg(tableProfileSystem);
	arg.add(columnDefDatabaseDir.defaultValue);
	arg.add(0); // enable_face_mysql
	arg.add(atoi(columnDefFaceRestPort.defaultValue));
	arg.add(atoi(columnDefEnableCopyOnDemand.defaultValue));
	dbAgent->insert(arg);
}

bool DBClientConfig::parseDBServer(const string &dbServer,
                                   string &host, size_t &port)
{
	size_t posColon = dbServer.find(":");
	if (posColon == string::npos) {
		host = dbServer;
		return true;
	}
	if (posColon == dbServer.size() - 1) {
		MLPL_ERR("A column must not be the tail: %s\n",
		         dbServer.c_str());
		return false;
	}
	host = string(dbServer, 0, posColon);
	port = atoi(&dbServer.c_str()[posColon+1]);
	return true;
}

bool DBClientConfig::canUpdateTargetServer(
  MonitoringServerInfo *monitoringServerInfo,
  const OperationPrivilege &privilege)
{
	if (privilege.has(OPPRVLG_UPDATE_ALL_SERVER))
		return true;

	if (!privilege.has(OPPRVLG_UPDATE_SERVER))
		return false;

	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->isAccessible(monitoringServerInfo->id, privilege, false);
}

bool DBClientConfig::canDeleteTargetServer(
  const ServerIdType &serverId, const OperationPrivilege &privilege)
{
	if (privilege.has(OPPRVLG_DELETE_ALL_SERVER))
		return true;

	if (!privilege.has(OPPRVLG_DELETE_SERVER))
		return false;

	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->isAccessible(serverId, privilege);
}

