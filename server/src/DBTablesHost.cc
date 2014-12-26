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
static const char *TABLE_NAME_HOSTGROUP_MEMBER  = "hostgroup_member";

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

const DBAgent::TableProfile tableProfileHostList =
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

static const int columnIndexesHostServerDefIdx[] = {
  IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
  IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHostServerDef[] = {
  {"HostServerDefIdx", (const int *)columnIndexesHostServerDefIdx, true},
  {NULL}
};

const DBAgent::TableProfile tableProfileServerHostDef =
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

const DBAgent::TableProfile tableProfileHostAccess =
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

const DBAgent::TableProfile tableProfileVMList =
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

static const int columnIndexesHostgroupListUniqId[] = {
  IDX_HOSTGROUP_LIST_SERVER_ID, IDX_HOSTGROUP_LIST_ID_IN_SERVER,
  DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHostgroupList[] = {
  {"HostgroupListUniqId", (const int *)columnIndexesHostgroupListUniqId, true},
  {NULL}
};

const DBAgent::TableProfile tableProfileHostgroupList =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOSTGROUP_LIST,
			    COLUMN_DEF_HOSTGROUP_LIST,
			    NUM_IDX_HOSTGROUP_LIST,
			    indexDefsHostgroupList);


static const ColumnDef COLUMN_DEF_HOSTGROUP_MEMBER[] = {
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
	SQL_KEY_NONE, // indexDefsHostgroupMember // keyType
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

static const int columnIndexesHostgroupMemberUniqId[] = {
  IDX_HOSTGROUP_MEMBER_SERVER_ID, IDX_HOSTGROUP_MEMBER_ID,
  DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHostgroupMember[] = {
  {"HostsHostgroupUniqId",
   (const int *)columnIndexesHostgroupMemberUniqId, true},
  {NULL}
};

const DBAgent::TableProfile tableProfileHostgroupMember =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOSTGROUP_MEMBER,
			    COLUMN_DEF_HOSTGROUP_MEMBER,
			    NUM_IDX_HOSTGROUP_MEMBER,
			    indexDefsHostgroupMember);

struct DBTablesHost::Impl
{
	Impl(void)
	{
	}

	virtual ~Impl()
	{
	}
};

static bool updateDB(DBAgent &dbAgent, const int &oldVer, void *data)
{
	if (oldVer == 1) {
		// In table ver.1 (on 14.09), this table is not used.
		// So we can drop it.
		dbAgent.dropTable(tableProfileHostList.name);
	}
	return true;
}

// ---------------------------------------------------------------------------
// HostsQueryOption
// ---------------------------------------------------------------------------
static const HostResourceQueryOption::Synapse synapseHostsQueryOption(
  tableProfileServerHostDef, IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER,
  IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
  tableProfileServerHostDef, IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER,
  true,
  tableProfileHostgroupMember,
  IDX_HOSTGROUP_MEMBER_SERVER_ID, IDX_HOSTGROUP_MEMBER_HOST_ID,
  IDX_HOSTGROUP_MEMBER_GROUP_ID);

struct HostsQueryOption::Impl {
	HostStatus status;

	Impl()
	: status(HOST_STAT_ALL)
	{
	}
};

HostsQueryOption::HostsQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseHostsQueryOption, userId),
  m_impl(new Impl())
{
}

HostsQueryOption::HostsQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseHostsQueryOption, dataQueryContext),
  m_impl(new Impl())
{
}

HostsQueryOption::~HostsQueryOption(void)
{
}

string HostsQueryOption::getCondition(void) const
{
	string condition = HostResourceQueryOption::getCondition();
	if (m_impl->status == HOST_STAT_ALL)
		return condition;
	if (!condition.empty())
		condition += " AND ";

	string columnName =
	  tableProfileServerHostDef.columnDefs[IDX_HOST_SERVER_HOST_DEF_HOST_STATUS].columnName;
	condition += StringUtils::sprintf("%s=%d",
	                                  columnName.c_str(), m_impl->status);
	return condition;
}

void HostsQueryOption::setStatus(const HostStatus &status)
{
	m_impl->status = status;
}

HostStatus HostsQueryOption::getStatus(void) const
{
	return m_impl->status;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBTablesHost::reset(void)
{
	getSetupInfo().initialized = false;
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
  const ServerHostDef &serverHostDef, const bool &useTransaction)
{
	HATOHOL_ASSERT(serverHostDef.hostId == AUTO_ASSIGNED_ID ||
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
				if (serverHostDef.hostId != AUTO_ASSIGNED_ID)
					assertHostIdConsistency(currSvHostDef);
				hostId = currSvHostDef.hostId;
			} else {
				bool shouldAddHost = false;
				if (serverHostDef.hostId == AUTO_ASSIGNED_ID) {
					shouldAddHost = true;
				} else {
					hostId = serverHostDef.hostId;
					shouldAddHost =
					  !hasHost(dbAgent, hostId);
				}

				if (shouldAddHost) {
					hostId = addHost(dbAgent,
					                 serverHostDef.hostId,
					                 serverHostDef.name);
					tookNewHostId = true;
				}
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

		HostIdType addHost(DBAgent &dbAgent, const HostIdType &hostId,
		                   const string &name)
		{
			DBAgent::InsertArg arg(tableProfileHostList);
			if (hostId == AUTO_ASSIGNED_ID)
				arg.add(AUTO_INCREMENT_VALUE);
			else
				arg.add(hostId);
			arg.add(name);
			arg.upsertOnDuplicate = (hostId != AUTO_ASSIGNED_ID);
			dbAgent.insert(arg);
			return (hostId == AUTO_ASSIGNED_ID) ?
			         dbAgent.getLastInsertId() : hostId;
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

		bool hasHost(DBAgent &dbAgent, const HostIdType &hostId)
		{
			string cond =
			  StringUtils::sprintf("%s=%" FMT_HOST_ID,
			    COLUMN_DEF_HOST_LIST[IDX_HOST_LIST_ID].columnName,
			    hostId);
			return dbAgent.isRecordExisting(TABLE_NAME_HOST_LIST,
		                                        cond);
		}

	} proc(serverHostDef);

	if (useTransaction)
		getDBAgent().runTransaction(proc);
	else
		proc(getDBAgent());

	return proc.hostId;
}

void DBTablesHost::upsertHosts(const ServerHostDefVect &serverHostDefs)
{
	struct Proc : public DBAgent::TransactionProc {
		DBTablesHost &dbHost;
		const ServerHostDefVect &serverHostDefs;

		Proc(DBTablesHost &_dbHost, const ServerHostDefVect &_serverHostDefs)
		: dbHost(_dbHost),
		  serverHostDefs(_serverHostDefs)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			ServerHostDefVectConstIterator svHostDefItr =
			  serverHostDefs.begin();
			for (; svHostDefItr != serverHostDefs.end(); ++svHostDefItr)
				dbHost.upsertHost(*svHostDefItr, false);
		}
	} proc(*this, serverHostDefs);
	getDBAgent().runTransaction(proc);
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

GenericIdType DBTablesHost::upsertHostgroup(const Hostgroup &hostgroup,
	                                    const bool &useTransaction)
{
	GenericIdType id;
	DBAgent::InsertArg arg(tableProfileHostgroupList);
	arg.add(hostgroup.id);
	arg.add(hostgroup.serverId);
	arg.add(hostgroup.idInServer);
	arg.add(hostgroup.name);
	arg.upsertOnDuplicate = true;

	DBAgent &dbAgent = getDBAgent();
	if (useTransaction) {
		dbAgent.runTransaction(arg, &id);
	} else {
		dbAgent.insert(arg);
		id = dbAgent.getLastInsertId();
	}

	return id;
}

void DBTablesHost::upsertHostgroups(const HostgroupVect &hostgroups)
{
	struct Proc : public DBAgent::TransactionProc {
		DBTablesHost &dbHost;
		const HostgroupVect &hostgroups;

		Proc(DBTablesHost &_dbHost, const HostgroupVect &_hostgroups)
		: dbHost(_dbHost),
		  hostgroups(_hostgroups)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			HostgroupVectConstIterator hostgrpItr =
			  hostgroups.begin();
			for (; hostgrpItr != hostgroups.end(); ++hostgrpItr)
				dbHost.upsertHostgroup(*hostgrpItr, false);
		}
	} proc(*this, hostgroups);
	getDBAgent().runTransaction(proc);
}

GenericIdType DBTablesHost::upsertHostgroupMember(
  const HostgroupMember &hostgroupMember, const bool &useTransaction)
{
	GenericIdType id;
	DBAgent::InsertArg arg(tableProfileHostgroupMember);
	arg.add(hostgroupMember.id);
	arg.add(hostgroupMember.serverId);
	arg.add(hostgroupMember.hostIdInServer);
	arg.add(hostgroupMember.hostgroupIdInServer);
	arg.upsertOnDuplicate = true;

	DBAgent &dbAgent = getDBAgent();
	if (useTransaction) {
		dbAgent.runTransaction(arg, &id);
	} else {
		dbAgent.insert(arg);
		id = dbAgent.getLastInsertId();
	}

	return id;
}

void DBTablesHost::upsertHostgroupMembers(
  const HostgroupMemberVect &hostgroupMembers)
{
	struct Proc : public DBAgent::TransactionProc {
		DBTablesHost &dbHost;
		const HostgroupMemberVect &hostgrpMembers;

		Proc(DBTablesHost &_dbHost,
		     const HostgroupMemberVect &_hostgrpMembers)
		: dbHost(_dbHost),
		  hostgrpMembers(_hostgrpMembers)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			HostgroupMemberVectConstIterator hgrpMemIt =
			  hostgrpMembers.begin();
			for (; hgrpMemIt != hostgrpMembers.end(); ++hgrpMemIt)
				dbHost.upsertHostgroupMember(*hgrpMemIt, false);
		}
	} proc(*this, hostgroupMembers);
	getDBAgent().runTransaction(proc);
}

HatoholError DBTablesHost::getVirtualMachines(
  HostIdVector &virtualMachines, const HostIdType &hypervisorHostId,
  const HostsQueryOption &option)
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
                                         const HostsQueryOption &option)
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
  const HostIdType &hostId, const HostsQueryOption &option)
{
	// Get the server ID and host ID (in the server)
	DBClientJoinBuilder builder(tableProfileServerHostDef);
	builder.add(IDX_HOST_SERVER_HOST_DEF_SERVER_ID);

	// TODO: add a column including host_id and use it
	builder.addTable(
	  tableProfileHostgroupMember, DBClientJoinBuilder::INNER_JOIN,
	  tableProfileServerHostDef, IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
	  IDX_HOSTGROUP_MEMBER_SERVER_ID,
	  tableProfileServerHostDef, IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER,
	  IDX_HOSTGROUP_MEMBER_HOST_ID);
	builder.add(IDX_HOSTGROUP_MEMBER_GROUP_ID);

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

HatoholError DBTablesHost::getServerHostDefs(
  ServerHostDefVect &svHostDefVect, const HostsQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileServerHostDef);
	arg.tableField = TABLE_NAME_SERVER_HOST_DEF;
	arg.add(IDX_HOST_SERVER_HOST_DEF_ID);
	arg.add(IDX_HOST_SERVER_HOST_DEF_HOST_ID);
	arg.add(IDX_HOST_SERVER_HOST_DEF_SERVER_ID);
	arg.add(IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);
	arg.add(IDX_HOST_SERVER_HOST_DEF_HOST_NAME);
	arg.add(IDX_HOST_SERVER_HOST_DEF_HOST_STATUS);

	getDBAgent().runTransaction(arg);

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	svHostDefVect.reserve(svHostDefVect.size() + grpList.size());

	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		ServerHostDef svHostDef;
		itemGroupStream >> svHostDef.id;
		itemGroupStream >> svHostDef.hostId;
		// TODO: select hosts in an SQL statement.
		if (option.getUserId() != USER_ID_SYSTEM) {
			if (!isAccessible(svHostDef.hostId, option))
				continue;
		}
		itemGroupStream >> svHostDef.serverId;
		itemGroupStream >> svHostDef.hostIdInServer;
		itemGroupStream >> svHostDef.name;
		itemGroupStream >> svHostDef.status;
		svHostDefVect.push_back(svHostDef);
	}
	return HTERR_OK;
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
		&tableProfileHostgroupMember,
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

