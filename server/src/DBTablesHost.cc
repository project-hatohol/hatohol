/*
 * Copyright (C) 2014-2015 Project Hatohol
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
#include <SeparatorInjector.h>
#include "DBTablesHost.h"
#include "ItemGroupStream.h"
#include "ThreadLocalDBCache.h"
#include "DBClientJoinBuilder.h"
#include "DBTermCStringProvider.h"
using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_HOST_LIST       = "host_list";
static const char *TABLE_NAME_SERVER_HOST_DEF = "server_host_def";
static const char *TABLE_NAME_HOST_ACCESS     = "host_access";
static const char *TABLE_NAME_VM_LIST         = "vm_list";
static const char *TABLE_NAME_HOSTGROUP_LIST  = "hostgroup_list";
static const char *TABLE_NAME_HOSTGROUP_MEMBER  = "hostgroup_member";

const int DBTablesHost::TABLES_VERSION = 3;

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
	"host_id_in_server",               // columnName
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
}, {
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

static const int columnIndexesHostgroupMemberUniqId[] = {
  IDX_HOSTGROUP_MEMBER_SERVER_ID,
  IDX_HOSTGROUP_MEMBER_HOST_ID_IN_SERVER,
  IDX_HOSTGROUP_MEMBER_GROUP_ID,
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
	bool storedHostsChanged;
	Impl(void)
	:storedHostsChanged(true)
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
	if (oldVer == 1 || oldVer == 2) {
		// In table ver.1 and 2 (on 14.09 and 14.12), these tables are
		// not used. So we can drop it.
		dbAgent.dropTable(tableProfileHostList.name);
		dbAgent.dropTable(tableProfileServerHostDef.name);
		dbAgent.dropTable(tableProfileHostgroupMember.name);
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
  IDX_HOSTGROUP_MEMBER_SERVER_ID, IDX_HOSTGROUP_MEMBER_HOST_ID_IN_SERVER,
  IDX_HOSTGROUP_MEMBER_GROUP_ID);

struct HostsQueryOption::Impl {
	set<HostStatus> statuses;

	Impl()
	: statuses({HOST_STAT_ALL})
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
	if (m_impl->statuses.count(HOST_STAT_ALL))
		return condition;

	string columnName =
	  tableProfileServerHostDef.columnDefs[IDX_HOST_SERVER_HOST_DEF_HOST_STATUS].columnName;
	string statuses;
	SeparatorInjector commaInjector(",");
	for (const auto &status : m_impl->statuses) {
		commaInjector(statuses);
		statuses += to_string(status);
	}
	addCondition(condition, StringUtils::sprintf("%s IN (%s)",
	             columnName.c_str(), statuses.c_str()));
	return condition;
}

void HostsQueryOption::setStatusSet(const set<HostStatus> &statuses)
{
	m_impl->statuses = statuses;
}

set<HostStatus> &HostsQueryOption::getStatusSet(void) const
{
	return m_impl->statuses;
}

// ---------------------------------------------------------------------------
// HostgroupsQueryOption
// ---------------------------------------------------------------------------
// TODO:
// HostgroupsQueryOption shouldn't inherit HostResourceQueryOption because
// it's not belong to a host.
static const HostResourceQueryOption::Synapse synapseHostgroupsQueryOption(
  tableProfileHostgroupList,
  IDX_HOSTGROUP_LIST_ID_IN_SERVER, IDX_HOSTGROUP_LIST_SERVER_ID,
  tableProfileHostgroupList,
  INVALID_COLUMN_IDX,
  false,
  tableProfileHostgroupMember, // TODO: It's dummy.
  IDX_HOSTGROUP_MEMBER_SERVER_ID, IDX_HOSTGROUP_MEMBER_HOST_ID_IN_SERVER,
  IDX_HOSTGROUP_MEMBER_GROUP_ID);

struct HostgroupsQueryOption::Impl {
	string hostgroupName;

	Impl()
	{
	}

	virtual ~Impl()
	{
	}
};

HostgroupsQueryOption::HostgroupsQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseHostgroupsQueryOption, userId),
  m_impl(new Impl())
{
}

HostgroupsQueryOption::HostgroupsQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseHostgroupsQueryOption, dataQueryContext),
  m_impl(new Impl())
{
}

HostgroupsQueryOption::~HostgroupsQueryOption(void)
{
}

void HostgroupsQueryOption::setTargetHostgroupName(const string &hostgroupName)
{
	m_impl->hostgroupName = hostgroupName;
}

string HostgroupsQueryOption::getTargetHostgroupName(void) const
{
	return m_impl->hostgroupName;
}

string HostgroupsQueryOption::getCondition(void) const
{
	string condition = HostResourceQueryOption::getCondition();
	if (!m_impl->hostgroupName.empty()) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		addCondition(condition,
		  StringUtils::sprintf("%s=%s",
		    COLUMN_DEF_HOSTGROUP_LIST[IDX_HOSTGROUP_LIST_NAME].columnName,
		    rhs(m_impl->hostgroupName)));
	}

	return condition;
}

std::string HostgroupsQueryOption::getHostgroupColumnName(const size_t &idx) const
{
	return getColumnNameCommon(tableProfileHostgroupList,
				   IDX_HOSTGROUP_LIST_ID_IN_SERVER);
}

bool HostgroupsQueryOption::isHostgroupUsed(void) const
{
	return false;
}

// ---------------------------------------------------------------------------
// HostgroupMembersQueryOption
// ---------------------------------------------------------------------------
static const HostResourceQueryOption::Synapse synapseHostgroupMembersQueryOption(
  tableProfileHostgroupMember,
  IDX_HOSTGROUP_MEMBER_ID, IDX_HOSTGROUP_MEMBER_SERVER_ID,
  tableProfileHostgroupMember,
  IDX_HOSTGROUP_MEMBER_HOST_ID_IN_SERVER, false,
  tableProfileHostgroupMember,
  IDX_HOSTGROUP_MEMBER_SERVER_ID, IDX_HOSTGROUP_MEMBER_HOST_ID_IN_SERVER,
  IDX_HOSTGROUP_MEMBER_GROUP_ID);

HostgroupMembersQueryOption::HostgroupMembersQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseHostgroupMembersQueryOption, userId)
{
}

HostgroupMembersQueryOption::HostgroupMembersQueryOption(
  DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseHostgroupMembersQueryOption, dataQueryContext)
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
	getDBAgent().runTransaction(arg, hostId);
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
	if (serverHostDef.status != HOST_STAT_SELF_MONITOR) {
		HATOHOL_ASSERT(!serverHostDef.name.empty(),
		               "Empty host name");
	}

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
			DBTermCStringProvider rhs(*dbAgent.getDBTermCodec());
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
			                  rhs(serverHostDef.hostIdInServer));
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

void DBTablesHost::upsertHosts(
  const ServerHostDefVect &serverHostDefs,
  HostHostIdMap *hostHostIdMapPtr, DBAgent::TransactionHooks *hooks)
{
	struct : public SeqTransactionProc<ServerHostDef, ServerHostDefVect> {
		function<void (const ServerHostDef &svHostDef)> func;
		void foreach(DBAgent &, const ServerHostDef &svHostDef) override
		{
			func(svHostDef);
		}
	} proc;
	proc.func = [&] (const ServerHostDef &svHostDef) {
		const HostIdType hostId = this->upsertHost(svHostDef, false);
		if (!hostHostIdMapPtr)
			return;
		(*hostHostIdMapPtr)[svHostDef.hostIdInServer] = hostId;
	};
	proc.init(this, &serverHostDefs);
	getDBAgent().runTransaction(proc, hooks);
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
	getDBAgent().runTransaction(arg, id);
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
	getDBAgent().runTransaction(arg, id);
	return id;
}

GenericIdType DBTablesHost::upsertVMInfo(const VMInfo &vmInfo,
                                         const bool &useTransaction)
{
	GenericIdType id;
	DBAgent::InsertArg arg(tableProfileVMList);
	arg.add(vmInfo.id);
	arg.add(vmInfo.hostId);
	arg.add(vmInfo.hypervisorHostId);
	arg.upsertOnDuplicate = true;

	DBAgent &dbAgent = getDBAgent();
	if (useTransaction) {
		dbAgent.runTransaction(arg, id);
	} else {
		dbAgent.insert(arg);
		id = dbAgent.getLastInsertId();
	}
	return id;
}

void DBTablesHost::upsertVMInfoVect(const VMInfoVect &vmInfoVect,
                                    DBAgent::TransactionHooks *hooks)
{
	struct : public SeqTransactionProc<VMInfo, VMInfoVect> {
		void foreach(DBAgent &dbAgent, const VMInfo &vminfo) override
		{
			get<DBTablesHost>().upsertVMInfo(vminfo, false);
		}
	} proc;
	proc.init(this, &vmInfoVect);
	getDBAgent().runTransaction(proc, hooks);
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
		dbAgent.runTransaction(arg, id);
	} else {
		dbAgent.insert(arg);
		id = dbAgent.getLastInsertId();
	}

	return id;
}

void DBTablesHost::upsertHostgroups(const HostgroupVect &hostgroups,
                                    DBAgent::TransactionHooks *hooks)
{
	struct : public SeqTransactionProc<Hostgroup, HostgroupVect> {
		void foreach(DBAgent &, const Hostgroup &hostgrp) override
		{
			get<DBTablesHost>().upsertHostgroup(hostgrp, false);
		}
	} proc;
	proc.init(this, &hostgroups);
	getDBAgent().runTransaction(proc, hooks);
}

HatoholError DBTablesHost::getHostgroups(HostgroupVect &hostgroups,
                                         const HostgroupsQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileHostgroupList);
	arg.add(IDX_HOSTGROUP_LIST_ID);
	arg.add(IDX_HOSTGROUP_LIST_SERVER_ID);
	arg.add(IDX_HOSTGROUP_LIST_ID_IN_SERVER);
	arg.add(IDX_HOSTGROUP_LIST_NAME);
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);

		Hostgroup hostgrp;
		itemGroupStream >> hostgrp.id;
		itemGroupStream >> hostgrp.serverId;
		itemGroupStream >> hostgrp.idInServer;
		itemGroupStream >> hostgrp.name;
		hostgroups.push_back(hostgrp);
	}

	return HTERR_OK;
}

HatoholError DBTablesHost::getServerHostGrpSetMap(
  ServerHostGrpSetMap &serverHostGrpSetMap,
  const HostgroupsQueryOption &option)
{
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::SelectExArg arg(tableProfileHostgroupList);
	arg.tableField = TABLE_NAME_HOSTGROUP_LIST;
	arg.add(IDX_HOSTGROUP_LIST_SERVER_ID);
	arg.add(IDX_HOSTGROUP_LIST_ID_IN_SERVER);
	arg.condition = option.getCondition();
	getDBAgent().runTransaction(arg);

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	for (auto itemGrp : grpList) {
		ServerIdType serverId;
		HostgroupIdType hostgroupId;
		ItemGroupStream itemGroupStream(itemGrp);
		itemGroupStream >> serverId;
		itemGroupStream >> hostgroupId;
		serverHostGrpSetMap[serverId].insert(hostgroupId);
	}

	return HTERR_OK;
}

static string makeIdListCondition(const GenericIdList &idList)
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_HOSTGROUP_LIST[IDX_HOSTGROUP_LIST_ID];
	SeparatorInjector commaInjector(",");
	condition = StringUtils::sprintf("%s in (", colId.columnName);
	for (auto id : idList) {
		commaInjector(condition);
		condition += StringUtils::sprintf("%" FMT_GEN_ID, id);
	}

	condition += ")";
	return condition;
}

static string makeConditionForDelete(const GenericIdList &idList)
{
	string condition = makeIdListCondition(idList);

	return condition;
}

HatoholError DBTablesHost::deleteHostgroupList(const GenericIdList &idList)
{
	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg arg;
		uint64_t numAffectedRows;

		TrxProc (void)
		: arg(tableProfileHostgroupList),
		  numAffectedRows(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(arg);
			numAffectedRows = dbAgent.getNumberOfAffectedRows();
		}
	} trx;
	trx.arg.condition = makeConditionForDelete(idList);
	getDBAgent().runTransaction(trx);

	// Check the result
	if (trx.numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         trx.numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
	}

	return HTERR_OK;
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
	arg.add(hostgroupMember.hostId);
	arg.upsertOnDuplicate = true;

	DBAgent &dbAgent = getDBAgent();
	if (useTransaction) {
		dbAgent.runTransaction(arg, id);
	} else {
		dbAgent.insert(arg);
		id = dbAgent.getLastInsertId();
	}

	return id;
}

void DBTablesHost::upsertHostgroupMembers(
  const HostgroupMemberVect &hostgroupMembers,
  DBAgent::TransactionHooks *hooks)
{
	struct :
	  public SeqTransactionProc<HostgroupMember, HostgroupMemberVect> {
		void foreach(DBAgent &, const HostgroupMember &hgrpMem) override
		{
			DBTablesHost &dbHost = get<DBTablesHost>();
			dbHost.upsertHostgroupMember(hgrpMem, false);
		}
	} proc;
	proc.init(this, &hostgroupMembers);
	getDBAgent().runTransaction(proc, hooks);
}

HatoholError DBTablesHost::getHostgroupMembers(
  HostgroupMemberVect &hostgroupMembers,
  const HostgroupMembersQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileHostgroupMember);
	arg.add(IDX_HOSTGROUP_MEMBER_ID);
	arg.add(IDX_HOSTGROUP_MEMBER_SERVER_ID);
	arg.add(IDX_HOSTGROUP_MEMBER_HOST_ID_IN_SERVER);
	arg.add(IDX_HOSTGROUP_MEMBER_GROUP_ID);
	arg.add(IDX_HOSTGROUP_MEMBER_HOST_ID);

	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	hostgroupMembers.reserve(grpList.size());
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		HostgroupMember hostgrpMember;
		hostgrpMember.id = itemGroupStream.read<GenericIdType>();
		itemGroupStream >> hostgrpMember.serverId;
		itemGroupStream >> hostgrpMember.hostIdInServer;
		itemGroupStream >> hostgrpMember.hostgroupIdInServer;
		itemGroupStream >> hostgrpMember.hostId;
		hostgroupMembers.push_back(hostgrpMember);
	}

	return HTERR_OK;
}

HatoholError DBTablesHost::deleteHostgroupMemberList(
  const GenericIdList &idList)
{
	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg arg;
		uint64_t numAffectedRows;

		TrxProc (void)
		: arg(tableProfileHostgroupMember),
		  numAffectedRows(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(arg);
			numAffectedRows = dbAgent.getNumberOfAffectedRows();
		}
	} trx;
	trx.arg.condition = makeConditionForDelete(idList);
	getDBAgent().runTransaction(trx);

	// Check the result
	if (trx.numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         trx.numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
	}

	return HTERR_OK;
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

HatoholError DBTablesHost::deleteVMInfoList(
  const GenericIdList &idList)
{
	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg arg;
		uint64_t numAffectedRows;

		TrxProc (void)
		: arg(tableProfileVMList),
		  numAffectedRows(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(arg);
			numAffectedRows = dbAgent.getNumberOfAffectedRows();
		}
	} trx;
	trx.arg.condition = makeConditionForDelete(idList);
	getDBAgent().runTransaction(trx);

	// Check the result
	if (trx.numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         trx.numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
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

	builder.addTable(
	  tableProfileHostgroupMember, DBClientJoinBuilder::INNER_JOIN,
	  IDX_HOST_SERVER_HOST_DEF_HOST_ID, IDX_HOSTGROUP_MEMBER_HOST_ID);
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
		HostgroupIdType hostgroupId;
		ItemGroupStream itemGroupStream(*itemGrpItr);
		itemGroupStream >> serverId;
		itemGroupStream >> hostgroupId;
		if (dbUser.isAccessible(serverId, hostgroupId, option))
			return true;
	}
	return false;
}

HatoholError DBTablesHost::getServerHostDefs(
  ServerHostDefVect &svHostDefVect, const HostsQueryOption &option)
{
	DBClientJoinBuilder builder(tableProfileServerHostDef);
	builder.add(IDX_HOST_SERVER_HOST_DEF_ID);
	builder.add(IDX_HOST_SERVER_HOST_DEF_HOST_ID);
	builder.add(IDX_HOST_SERVER_HOST_DEF_SERVER_ID);
	builder.add(IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);
	builder.add(IDX_HOST_SERVER_HOST_DEF_HOST_NAME);
	builder.add(IDX_HOST_SERVER_HOST_DEF_HOST_STATUS);

	if (option.isHostgroupUsed()) {
		builder.addTable(
		  tableProfileHostgroupMember, DBClientJoinBuilder::INNER_JOIN,
		  IDX_HOST_SERVER_HOST_DEF_HOST_ID,
		  IDX_HOSTGROUP_MEMBER_HOST_ID);
	}

	DBAgent::SelectExArg &arg = builder.build();
	if (option.isHostgroupUsed()) {
		// TODO: FIX This low level implementation is temporary
		// We should make a framework to use a sub query
		string matchCond = StringUtils::sprintf("%s=%s",
		  tableProfileServerHostDef.getFullColumnName(
		    IDX_HOST_SERVER_HOST_DEF_HOST_ID).c_str(),
		  tableProfileHostgroupMember.getFullColumnName(
		    IDX_HOSTGROUP_MEMBER_HOST_ID).c_str());

		arg.tableField = tableProfileServerHostDef.name;
		arg.condition = "EXISTS (SELECT * from ";
		arg.condition += tableProfileHostgroupMember.name;
		arg.condition += " WHERE (";
		arg.condition += matchCond;
		arg.condition += ") AND (";
		arg.condition += option.getCondition();
		arg.condition += "))";
	} else {
		arg.condition = option.getCondition();
	}
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
		itemGroupStream >> svHostDef.serverId;
		itemGroupStream >> svHostDef.hostIdInServer;
		itemGroupStream >> svHostDef.name;
		itemGroupStream >> svHostDef.status;
		svHostDefVect.push_back(svHostDef);
	}
	return HTERR_OK;
}

bool DBTablesHost::wasStoredHostsChanged(void)
{
	return m_impl->storedHostsChanged;
}

static bool isHostNameChanged(
  ServerHostDef newSvHostDef, map<string, const ServerHostDef *> currValidHosts)
{
	auto hostDefItr = currValidHosts.find(newSvHostDef.hostIdInServer);
	if (hostDefItr != currValidHosts.end()) {
		if (hostDefItr->second->name != newSvHostDef.name) {
			return true;
		}
	}
	return false;
}

size_t DBTablesHost::getNumberOfHosts(HostsQueryOption &option)
{
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    option.getColumnName(IDX_HOST_SERVER_HOST_DEF_ID).c_str());
	DBAgent::SelectExArg arg(tableProfileServerHostDef);
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	getDBAgent().runTransaction(arg);

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

HatoholError DBTablesHost::syncHosts(
  const ServerHostDefVect &svHostDefs, const ServerIdType &serverId,
  HostHostIdMap *hostHostIdMapPtr, DBAgent::TransactionHooks *hooks)
{
	// Make a set that contains current hosts records
	HostsQueryOption option(USER_ID_SYSTEM);
	option.setStatusSet({HOST_STAT_NORMAL});
	option.setTargetServerId(serverId);

	ServerHostDefVect _currHosts;
	HatoholError err = getServerHostDefs(_currHosts, option);
	if (err != HTERR_OK)
		return err;
	const ServerHostDefVect currHosts = move(_currHosts);

	map<string, const ServerHostDef *> currValidHosts;
	for (auto& svHostDef : currHosts) {
		currValidHosts[svHostDef.hostIdInServer] = &svHostDef;
	}

	// Pick up hosts to be added.
	ServerHostDefVect serverHostDefs;
	for (auto& newSvHostDef : svHostDefs) {
		if (!isHostNameChanged(newSvHostDef, currValidHosts) &&
		    currValidHosts.erase(newSvHostDef.hostIdInServer) >= 1) {
			// The host already exists or unmodified. We have nothing to do.
			continue;
		}
		serverHostDefs.push_back(move(newSvHostDef));
	}

	// Add hosts to be marked as invalid
	auto invalidHostMap = move(currValidHosts);
	for (auto invalidHostPair : invalidHostMap) {
		ServerHostDef invalidHost = *invalidHostPair.second; // make a copy
		invalidHost.status = HOST_STAT_REMOVED;
		serverHostDefs.push_back(invalidHost);
	}

	if (serverHostDefs.empty()){
		m_impl->storedHostsChanged = true;
		return HTERR_OK;
	}
	upsertHosts(serverHostDefs, hostHostIdMapPtr, hooks);
	m_impl->storedHostsChanged = false;
	return HTERR_OK;
}

static bool isHostgroupNameChanged(
  Hostgroup hostgroup, map<HostgroupIdType, const Hostgroup *> currentHostgroupMap)
{
	auto hostgroupItr = currentHostgroupMap.find(hostgroup.idInServer);
	if (hostgroupItr != currentHostgroupMap.end()) {
		if (hostgroupItr->second->name != hostgroup.name) {
			return true;
		}
	}
	return false;
}

HatoholError DBTablesHost::syncHostgroups(
  const HostgroupVect &incomingHostgroups,
  const ServerIdType &serverId, DBAgent::TransactionHooks *hooks)
{
	HostgroupsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverId);
	HostgroupVect _currHostgroups;
	HatoholError err = getHostgroups(_currHostgroups, option);
	if (err != HTERR_OK)
		return err;
	const HostgroupVect currHostgroups = move(_currHostgroups);

	map<HostgroupIdType, const Hostgroup *> currentHostgroupMap;
	for (auto& hostgroup : currHostgroups) {
		currentHostgroupMap[hostgroup.idInServer] = &hostgroup;
	}

	// Pick up hostgroups to be added.
	HostgroupVect serverHostgroups;
	for (auto hostgroup : incomingHostgroups) {
		if (!isHostgroupNameChanged(hostgroup, currentHostgroupMap) &&
		    currentHostgroupMap.erase(hostgroup.idInServer) >= 1) {
			// If the hostgroup already exists or unmodified,
			// we have nothing to do.
			continue;
		}
		serverHostgroups.push_back(move(hostgroup));
	}

	GenericIdList invalidHostgroupIdList;
	map<HostgroupIdType, const Hostgroup *> invalidHostgroupMap =
		move(currentHostgroupMap);
	for (auto invalidHostgroupPair : invalidHostgroupMap) {
		Hostgroup invalidHostgroup = *invalidHostgroupPair.second;
		invalidHostgroupIdList.push_back(invalidHostgroup.id);
	}
	if (invalidHostgroupIdList.size() > 0)
		err = deleteHostgroupList(invalidHostgroupIdList);
	if (serverHostgroups.size() > 0)
		upsertHostgroups(serverHostgroups, hooks);
	return err;
}

HatoholError DBTablesHost::syncHostgroupMembers(
  const HostgroupMemberVect &incomingHostgroupMembers,
  const ServerIdType &serverId,
  DBAgent::TransactionHooks *hooks)
{
	HostgroupMembersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverId);
	HostgroupMemberVect _currHostgroupMembers;
	HatoholError err = getHostgroupMembers(_currHostgroupMembers, option);
	if (err != HTERR_OK)
		return err;
	const HostgroupMemberVect currHostgroupMembers = move(_currHostgroupMembers);
	map<HostgroupIdType, map<LocalHostIdType, const HostgroupMember *> > currentHostgroupMapHostsMap;
	for (auto& hostgroupMember : currHostgroupMembers) {
		currentHostgroupMapHostsMap[hostgroupMember.hostgroupIdInServer][hostgroupMember.hostIdInServer] = &hostgroupMember;
	}

	//Pick up hostgroupMember to be added.
	HostgroupMemberVect serverHostgroupMembers;
	for (auto hostgroupMember : incomingHostgroupMembers) {
		auto groupIdItr = currentHostgroupMapHostsMap.find(hostgroupMember.hostgroupIdInServer);
		if (groupIdItr != currentHostgroupMapHostsMap.end()) {
			auto& currentHostgroupMap = groupIdItr->second;
			if (currentHostgroupMap.erase(hostgroupMember.hostIdInServer) >= 1) {
				continue;
			}
		}
		serverHostgroupMembers.push_back(move(hostgroupMember));
	}

	GenericIdList invalidHostgroupMemberIdList;
	auto invalidHostgroupMapHostsMap = move(currentHostgroupMapHostsMap);
	for (auto invalidHostgroupMap :  invalidHostgroupMapHostsMap) {
		for (auto invalidHostgroupPair : invalidHostgroupMap.second) {
			auto invalidHostgroupMember = *invalidHostgroupPair.second;
			invalidHostgroupMemberIdList.push_back(invalidHostgroupMember.id);
		}
	}
	if (invalidHostgroupMemberIdList.size() > 0)
		err = deleteHostgroupMemberList(invalidHostgroupMemberIdList);
	if (serverHostgroupMembers.size() > 0)
		upsertHostgroupMembers(serverHostgroupMembers, hooks);
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
		DBTablesId::HOST,
		TABLES_VERSION,
		NUM_TABLE_INFO,
		TABLE_INFO,
		&updateDB,
	};
	return SETUP_INFO;
}
