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

#include <cstdio>
#include "DBTablesHost.h"
#include "ItemGroupStream.h"
#include "ThreadLocalDBCache.h"
#include "DBClientJoinBuilder.h"
using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_HOST_LIST       = "host_list";
static const char *TABLE_NAME_SERVER_HOST_DEF = "server_host_def";
static const char *TABLE_NAME_HOST_ACCESS     = "host_access";
static const char *TABLE_NAME_VM_LIST         = "vm_list";
static const char *TABLE_NAME_HOSTGROUP_LIST  = "hostgroup_list";
static const char *TABLE_NAME_HOST_HOSTGROUP  = "host_hostgroup";

const int DBTablesHost::TABLES_VERSION = 2;

const uint64_t NO_HYPERVISOR = -1;
const size_t MAX_HOST_NAME_LENGTH =  255;

void operator>>(ItemGroupStream &itemGroupStream, HostStatus &rhs)
{
	rhs = itemGroupStream.read<int, HostStatus>();
}


static const ColumnDef COLUMN_DEF_HOST_LIST[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_HOST_LIST_ID,
	IDX_HOST_LIST_NAME,
	NUM_IDX_HOST_LIST
};

static const DBAgent::TableProfile tableProfileHostList =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOST_LIST,
			    COLUMN_DEF_HOST_LIST,
			    NUM_IDX_HOST_LIST);

static const ColumnDef COLUMN_DEF_SERVER_HOST_DEF[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	// Host ID in host_list table.
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
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
}, {
	// The type of this column is a string so that a UUID string
	// can also be stored.
	"host_id_in_server",               // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	// The name the monitoring server uses
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
},
};

enum {
	IDX_HOST_SERVER_HOST_DEF_ID,
	IDX_HOST_SERVER_HOST_DEF_HOST_ID,
	IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
	IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER,
	IDX_HOST_SERVER_HOST_DEF_HOST_NAME,
	IDX_HOST_SERVER_HOST_DEF_HOST_STATUS,
	NUM_IDX_SERVER_HOST_DEF
};

static const int columnIndexesHostServerDefIdx[] = {
  IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
  IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHostServerDef[] = {
  {"HostServerDefIdx", (const int *)columnIndexesHostServerDefIdx, false},
  {NULL}
};

static const DBAgent::TableProfile tableProfileServerHostDef =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_SERVER_HOST_DEF,
			    COLUMN_DEF_SERVER_HOST_DEF,
			    NUM_IDX_SERVER_HOST_DEF,
			    indexDefsHostServerDef);

// We manage multiple IP adresses and host naems for one host.
// So the following are defined in the independent table.
static const ColumnDef COLUMN_DEF_HOST_ACCESS[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	// Any of IPv4, IPv6 and FQDN
	"ip_addr_or_fqdn",                 // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	// The value in this column offers a hint to decide which IP/FQDN
	// should be used. The greater number has higher priority.
	"priority",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_HOST_ACCESS_ID,
	IDX_HOST_ACCESS_HOST_ID,
	IDX_HOST_ACCESS_IP_ADDR_OR_FQDN,
	IDX_HOST_ACCESS_PRIORITY,
	NUM_IDX_HOST_ACCESS
};

static const DBAgent::TableProfile tableProfileHostAccess =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOST_ACCESS,
			    COLUMN_DEF_HOST_ACCESS,
			    NUM_IDX_HOST_ACCESS);

static const ColumnDef COLUMN_DEF_VM_LIST[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"hypervisor_host_id",              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_HOST_VM_LIST_ID,
	IDX_HOST_VM_LIST_HOST_ID,
	IDX_HOST_VM_LIST_HYPERVISOR_HOST_ID,
	NUM_IDX_VM_LIST
};

static const DBAgent::TableProfile tableProfileVMList =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_VM_LIST,
			    COLUMN_DEF_VM_LIST,
			    NUM_IDX_VM_LIST);

static const ColumnDef COLUMN_DEF_HOSTGROUP_LIST[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsHostgroups // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"id_in_server",                    // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_HOSTGROUP_LIST_ID,
	IDX_HOSTGROUP_LIST_SERVER_ID,
	IDX_HOSTGROUP_LIST_ID_IN_SERVER,
	IDX_HOSTGROUP_LIST_NAME,
	NUM_IDX_HOSTGROUP_LIST,
};

static const int columnIndexesHostgroupListUniqId[] = {
  IDX_HOSTGROUP_LIST_SERVER_ID, IDX_HOSTGROUP_LIST_ID_IN_SERVER,
  DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHostgroupList[] = {
  {"HostgroupListUniqId", (const int *)columnIndexesHostgroupListUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileHostgroupList =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOSTGROUP_LIST,
			    COLUMN_DEF_HOSTGROUP_LIST,
			    NUM_IDX_HOSTGROUP_LIST,
			    indexDefsHostgroupList);


static const ColumnDef COLUMN_DEF_HOST_HOSTGROUP[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsHostHostgroup // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_HOST_HOSTGROUP_ID,
	IDX_HOST_HOSTGROUP_SERVER_ID,
	IDX_HOST_HOSTGROUP_HOST_ID,
	IDX_HOST_HOSTGROUP_GROUP_ID,
	NUM_IDX_HOST_HOSTGROUP,
};

static const int columnIndexesHostHostgroupUniqId[] = {
  IDX_HOST_HOSTGROUP_SERVER_ID, IDX_HOST_HOSTGROUP_ID,
  DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHostHostgroup[] = {
  {"HostsHostgroupUniqId",
   (const int *)columnIndexesHostHostgroupUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileHostHostgroup =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOST_HOSTGROUP,
			    COLUMN_DEF_HOST_HOSTGROUP,
			    NUM_IDX_HOST_HOSTGROUP,
			    indexDefsHostHostgroup);

struct DBTablesHost::Impl
{
	Impl(void)
	{
	}

	virtual ~Impl()
	{
	}
};

static bool updateDB(
  DBAgent &dbAgent, const DBTables::Version &oldPackedVer, void *data)
{
	const int &oldVer = oldPackedVer.minorVer;
	if (oldVer == 1) {
		// In table ver.1 (on 14.09), this table is not used.
		// So we can drop it.
		dbAgent.dropTable(tableProfileHostList.name);
	}
	return true;
}

// ---------------------------------------------------------------------------
// HostQueryOption
// ---------------------------------------------------------------------------
HostQueryOption::HostQueryOption(const UserIdType &userId)
: DataQueryOption(userId)
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBTablesHost::reset(void)
{
	getSetupInfo().initialized = false;
}

const DBTables::SetupInfo &DBTablesHost::getConstSetupInfo(void)
{
	return getSetupInfo();
}

DBTablesHost::DBTablesHost(DBAgent &dbAgent)
: DBTables(dbAgent, getSetupInfo()),
  m_impl(new Impl())
{
}

DBTablesHost::~DBTablesHost()
{
}

HostIdType DBTablesHost::addHost(const string &name)
{
	HostIdType hostId;
	DBAgent::InsertArg arg(tableProfileHostList);
	arg.add(AUTO_INCREMENT_VALUE);
	arg.add(name);
	getDBAgent().runTransaction(arg, &hostId);
	return hostId;
}

static void setupUpsertArgOfServerHostDef(
  DBAgent::InsertArg &arg, const ServerHostDef &serverHostDef,
  const HostIdType &hostId)
{
	arg.add(serverHostDef.id);
	arg.add(hostId);
	arg.add(serverHostDef.serverId);
	arg.add(serverHostDef.hostIdInServer);
	arg.add(serverHostDef.name);
	arg.add(serverHostDef.status);
	arg.upsertOnDuplicate = true;
}

static string setupFmtCondSelectSvHostDef(void)
{
	return StringUtils::sprintf("%s=%%" FMT_SERVER_ID " AND %s=%%s",
	         COLUMN_DEF_SERVER_HOST_DEF[
	           IDX_HOST_SERVER_HOST_DEF_SERVER_ID].columnName,
	         COLUMN_DEF_SERVER_HOST_DEF[
	           IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER].columnName);
}

HostIdType DBTablesHost::upsertHost(
  const ServerHostDef &serverHostDef)
{
	HATOHOL_ASSERT(serverHostDef.hostId == UNKNOWN_HOST_ID ||
	               serverHostDef.hostId >= 0,
	               "Invalid host ID: %" FMT_HOST_ID, serverHostDef.hostId);
	HATOHOL_ASSERT(!serverHostDef.name.empty(),
	               "Empty host name: %s", serverHostDef.name.c_str());

	struct Proc : public DBAgent::TransactionProc {
		const ServerHostDef &serverHostDef;
		DBAgent::InsertArg serverHostDefArg;
		string fmtCondSelectSvHostDef;
		HostIdType hostId;

		Proc(const ServerHostDef &_serverHostDef)
		: serverHostDef(_serverHostDef),
		  serverHostDefArg(tableProfileServerHostDef),
		  fmtCondSelectSvHostDef(setupFmtCondSelectSvHostDef()),
		  hostId(INVALID_HOST_ID)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			ServerHostDef currSvHostDef;
			bool tookNewHostId = false;
			const bool found = selectServerHostDef(dbAgent,
			                                       currSvHostDef);
			if (found) {
				if (!isChanged(serverHostDef, currSvHostDef)) {
					hostId = currSvHostDef.hostId;
					return;
				}
				if (serverHostDef.hostId == UNKNOWN_HOST_ID)
					hostId = currSvHostDef.hostId;
				assertHostIdConsistency(currSvHostDef);
			} else {
				hostId = addHost(dbAgent, serverHostDef.name);
				tookNewHostId = true;
			}

			setupUpsertArgOfServerHostDef(serverHostDefArg,
			                              serverHostDef, hostId);
			dbAgent.insert(serverHostDefArg);
			dbAgent.getLastInsertId();
			if (dbAgent.lastUpsertDidUpdate() && tookNewHostId) {
				// This path is rare.
				// [Assumed scenario]
				// Other session tried to query the record
				// and didn't find it. Then the insert() was
				// invoked right before that of this session.
				removeHost(dbAgent, hostId);
				HATOHOL_ASSERT(
				  selectServerHostDef(dbAgent, currSvHostDef),
				  "Record is not found.");
				hostId = currSvHostDef.hostId;
			}
		}

		/**
		 * Get the record whose server ID and the host ID in server are
		 * equals to serverHostDef.serverId and
		 * serverHostDef.hostIdInServer.
		 *
		 * @param svHostDef The obtained record is saved here
		 *
		 * @return true if the record is found. Otherwise false.
		 */
		bool selectServerHostDef(DBAgent &dbAgent,
		                         ServerHostDef &svHostDef)
		{
			DBAgent::SelectExArg arg(tableProfileServerHostDef);
			arg.add(IDX_HOST_SERVER_HOST_DEF_ID);
			arg.add(IDX_HOST_SERVER_HOST_DEF_HOST_ID);
			arg.add(IDX_HOST_SERVER_HOST_DEF_SERVER_ID);
			arg.add(IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);
			arg.add(IDX_HOST_SERVER_HOST_DEF_HOST_NAME);
			arg.add(IDX_HOST_SERVER_HOST_DEF_HOST_STATUS);
			arg.condition = StringUtils::sprintf(
			                  fmtCondSelectSvHostDef.c_str(),
			                  serverHostDef.serverId,
			                  serverHostDef.hostIdInServer.c_str());
			dbAgent.select(arg);

			const ItemGroupList &grpList =
			  arg.dataTable->getItemGroupList();
			if (grpList.empty())
				return false;
			const size_t numHosts = grpList.size();
			if (numHosts> 1) {
				MLPL_BUG(
				  "Found %zd records. ServerID: %"
				  FMT_SERVER_ID ", host ID in server: %s\n",
				  numHosts, serverHostDef.serverId,
				  serverHostDef.hostIdInServer.c_str());
			}
			ItemGroupStream itemGroupStream(*grpList.begin());
			itemGroupStream >> svHostDef.id;
			itemGroupStream >> svHostDef.hostId;
			itemGroupStream >> svHostDef.serverId;
			itemGroupStream >> svHostDef.hostIdInServer;
			itemGroupStream >> svHostDef.name;
			itemGroupStream >> svHostDef.status;
			return true;
		}

		bool isChanged(const ServerHostDef &shd0,
		               const ServerHostDef &shd1)
		{
			// serverId and hostIdInServer is not checked because
			// they are used as a condition in selectServerHostDef()
			// And hostIds should be the same if a combination of
			// serverId and hostIdInServer is identical.
			if (shd0.name != shd1.name)
				return true;
			if (shd0.status != shd1.status)
				return true;
			return false;
		}

		HostIdType addHost(DBAgent &dbAgent, const string &name)
		{
			DBAgent::InsertArg arg(tableProfileHostList);
			arg.add(AUTO_INCREMENT_VALUE);
			arg.add(name);
			dbAgent.insert(arg);
			return dbAgent.getLastInsertId();
		}

		void removeHost(DBAgent &dbAgent, const HostIdType &hostId)
		{
			DBAgent::DeleteArg arg(tableProfileHostList);
			arg.condition =
			  StringUtils::sprintf("%s=%" FMT_HOST_ID,
			    COLUMN_DEF_HOST_LIST[IDX_HOST_LIST_ID].columnName,
			    hostId);
			dbAgent.deleteRows(arg);
		}

		void assertHostIdConsistency(const ServerHostDef currSvHostDef)
		{
			HATOHOL_ASSERT(
			  currSvHostDef.hostId == serverHostDef.hostId,
			  "Host ID inconsistent: DB: %" FMT_HOST_ID ", "
			  "Input: %" FMT_HOST_ID,
			  currSvHostDef.hostId, serverHostDef.hostId);
		}

	} transaction(serverHostDef);
	getDBAgent().runTransaction(transaction);

	return transaction.hostId;
}

GenericIdType DBTablesHost::upsertServerHostDef(
  const ServerHostDef &serverHostDef)
{
	GenericIdType id;
	DBAgent::InsertArg arg(tableProfileServerHostDef);
	arg.add(serverHostDef.id);
	arg.add(serverHostDef.hostId);
	arg.add(serverHostDef.serverId);
	arg.add(serverHostDef.hostIdInServer);
	arg.add(serverHostDef.name);
	arg.add(serverHostDef.status);
	arg.upsertOnDuplicate = true;
	getDBAgent().runTransaction(arg, &id);
	return id;
}

GenericIdType DBTablesHost::upsertHostAccess(const HostAccess &hostAccess)
{
	GenericIdType id;
	DBAgent::InsertArg arg(tableProfileHostAccess);
	arg.add(hostAccess.id);
	arg.add(hostAccess.hostId);
	arg.add(hostAccess.ipAddrOrFQDN);
	arg.add(hostAccess.priority);
	arg.upsertOnDuplicate = true;
	getDBAgent().runTransaction(arg, &id);
	return id;
}

GenericIdType DBTablesHost::upsertVMInfo(const VMInfo &vmInfo)
{
	GenericIdType id;
	DBAgent::InsertArg arg(tableProfileVMList);
	arg.add(vmInfo.id);
	arg.add(vmInfo.hostId);
	arg.add(vmInfo.hypervisorHostId);
	arg.upsertOnDuplicate = true;
	getDBAgent().runTransaction(arg, &id);
	return id;
}

GenericIdType DBTablesHost::upsertHostgroup(const Hostgroup &hostgroup)
{
	GenericIdType id;
	DBAgent::InsertArg arg(tableProfileHostgroupList);
	arg.add(hostgroup.id);
	arg.add(hostgroup.serverId);
	arg.add(hostgroup.idInServer);
	arg.add(hostgroup.name);
	arg.upsertOnDuplicate = true;
	getDBAgent().runTransaction(arg, &id);
	return id;
}

GenericIdType DBTablesHost::upsertHostHostgroup(
  const HostHostgroup &hostHostgroup)
{
	GenericIdType id;
	DBAgent::InsertArg arg(tableProfileHostHostgroup);
	arg.add(hostHostgroup.id);
	arg.add(hostHostgroup.serverId);
	arg.add(hostHostgroup.hostIdInServer);
	arg.add(hostHostgroup.hostgroupIdInServer);
	arg.upsertOnDuplicate = true;
	getDBAgent().runTransaction(arg, &id);
	return id;
}

HatoholError DBTablesHost::getVirtualMachines(
  HostIdVector &virtualMachines, const HostIdType &hypervisorHostId,
  const HostQueryOption &option)
{
	if (option.getUserId() == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	DBAgent::SelectExArg arg(tableProfileVMList);
	arg.tableField = TABLE_NAME_VM_LIST;
	arg.add(IDX_HOST_VM_LIST_HOST_ID);
	const UserIdType userId = option.getUserId();
	arg.condition = StringUtils::sprintf(
	  "%s=%" FMT_HOST_ID,
	  COLUMN_DEF_VM_LIST[IDX_HOST_VM_LIST_HYPERVISOR_HOST_ID].columnName,
	  hypervisorHostId);
	getDBAgent().runTransaction(arg);

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		HostIdType hostId;
		ItemGroupStream itemGroupStream(*itemGrpItr);
		itemGroupStream >> hostId;
		if (userId != USER_ID_SYSTEM) {
			if (!isAccessible(hostId, option))
				continue;
		}
		virtualMachines.push_back(hostId);
	}

	return HTERR_OK;
}

HatoholError DBTablesHost::getHypervisor(HostIdType &hypervisorHostId,
                                         const HostIdType &hostId,
                                         const HostQueryOption &option)
{
	if (option.getUserId() == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	DBAgent::SelectExArg arg(tableProfileVMList);
	arg.tableField = TABLE_NAME_VM_LIST;
	arg.add(IDX_HOST_VM_LIST_HYPERVISOR_HOST_ID);
	const UserIdType userId = option.getUserId();
	arg.condition = StringUtils::sprintf(
	  "%s=%" FMT_HOST_ID,
	  COLUMN_DEF_VM_LIST[IDX_HOST_VM_LIST_HOST_ID].columnName,
	  hostId);
	getDBAgent().runTransaction(arg);

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	size_t numElem = grpList.size();
	if (numElem == 0)
		return HTERR_NOT_FOUND_HYPERVISOR;
	if (numElem >= 2) {
		MLPL_WARN("Found %zd hypervisors for host: %" FMT_HOST_ID "."
		          "We just use the first one.\n",
		          numElem, hostId);
	}

	HostIdType hypHostId;
	ItemGroupStream itemGroupStream(*grpList.begin());
	itemGroupStream >> hypHostId;
	if (userId != USER_ID_SYSTEM) {
		if (!isAccessible(hypHostId, option))
			return HTERR_NOT_FOUND_HYPERVISOR;
	}
	hypervisorHostId = hypHostId;

	return HTERR_OK;
}

bool DBTablesHost::isAccessible(
  const HostIdType &hostId, const HostQueryOption &option)
{
	// Get the server ID and host ID (in the server)
	DBClientJoinBuilder builder(tableProfileServerHostDef);
	builder.add(IDX_HOST_SERVER_HOST_DEF_SERVER_ID);

	// TODO: add a column including host_id and use it
	builder.addTable(
	  tableProfileHostHostgroup, DBClientJoinBuilder::INNER_JOIN,
	  tableProfileServerHostDef, IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
	  IDX_HOST_HOSTGROUP_SERVER_ID,
	  tableProfileServerHostDef, IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER,
	  IDX_HOST_HOSTGROUP_HOST_ID);
	builder.add(IDX_HOST_HOSTGROUP_GROUP_ID);

	DBAgent::SelectExArg &arg = builder.build();
	arg.condition = StringUtils::sprintf(
	  "%s=%" FMT_HOST_ID,
	  tableProfileServerHostDef.getFullColumnName(IDX_HOST_SERVER_HOST_DEF_HOST_ID).c_str(), hostId);
	getDBAgent().runTransaction(arg);

	// Check accessibility for each server
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ServerIdType serverId;
		string hostgroupIdInServer;
		ItemGroupStream itemGroupStream(*itemGrpItr);
		itemGroupStream >> serverId;
		itemGroupStream >> hostgroupIdInServer;
		// TODO: DBTablesUser should handle hostgroup ID as a string
		HostgroupIdType hostgroupId;
		int ret = sscanf(hostgroupIdInServer.c_str(),
		                 "%" FMT_HOST_GROUP_ID, &hostgroupId);
		HATOHOL_ASSERT(ret == 1, "ret: %d, str: %s\n",
		               ret, hostgroupIdInServer.c_str());
		if (dbUser.isAccessible(serverId, hostgroupId, option))
			return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
DBTables::SetupInfo &DBTablesHost::getSetupInfo(void)
{
	//
	// set database info
	//
	static const TableSetupInfo TABLE_INFO[] = {
	{
		&tableProfileHostList,
	}, {
		&tableProfileServerHostDef,
	}, {
		&tableProfileHostAccess,
	}, {
		&tableProfileVMList,
	}, {
		&tableProfileHostgroupList,
	}, {
		&tableProfileHostHostgroup,
	}
	};
	static const size_t NUM_TABLE_INFO =
	  ARRAY_SIZE(TABLE_INFO);

	static SetupInfo SETUP_INFO = {
		DB_TABLES_ID_HOST,
		TABLES_VERSION,
		NUM_TABLE_INFO,
		TABLE_INFO,
		&updateDB,
	};
	return SETUP_INFO;
}

