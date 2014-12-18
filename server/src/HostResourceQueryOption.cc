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

#include "Params.h"
#include "HostResourceQueryOption.h"
#include "DBTablesMonitoring.h"
#include "DBAgentSQLite3.h" // TODO: Shouldn't use explicitly
#include "DBHatohol.h"
using namespace std;
using namespace mlpl;

// *** TODO: use OptionTermGenrator to make SQL's clauses ****

// ---------------------------------------------------------------------------
// Synapse
// ---------------------------------------------------------------------------
HostResourceQueryOption::Synapse::Synapse(
  const DBAgent::TableProfile &_tableProfile,
  const size_t &_selfIdColumnIdx,
  const size_t &_serverIdColumnIdx,
  const DBAgent::TableProfile &_hostTableProfile,
  const size_t &_hostIdColumnIdx,
  const bool &_needToJoinHostgroup,
  const DBAgent::TableProfile &_hostgroupMapTableProfile,
  const size_t &_hostgroupMapServerIdColumnIdx,
  const size_t &_hostgroupMapHostIdColumnIdx,
  const size_t &_hostgroupMapGroupIdColumnIdx)
: tableProfile(_tableProfile),
  selfIdColumnIdx(_selfIdColumnIdx),
  serverIdColumnIdx(_serverIdColumnIdx),
  hostTableProfile(_hostTableProfile),
  hostIdColumnIdx(_hostIdColumnIdx),
  needToJoinHostgroup(_needToJoinHostgroup),
  hostgroupMapTableProfile(_hostgroupMapTableProfile),
  hostgroupMapServerIdColumnIdx(_hostgroupMapServerIdColumnIdx),
  hostgroupMapHostIdColumnIdx(_hostgroupMapHostIdColumnIdx),
  hostgroupMapGroupIdColumnIdx(_hostgroupMapGroupIdColumnIdx)
{
}

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct HostResourceQueryOption::Impl {
	static const DBTermCodec *dbTermCodec;
	const Synapse  &synapse;
	ServerIdType    targetServerId;
	HostIdType      targetHostId;
	HostgroupIdType targetHostgroupId;
	bool            filterDataOfDefunctServers;

	Impl(const Synapse &_synapse)
	: synapse(_synapse),
	  targetServerId(ALL_SERVERS),
	  targetHostId(ALL_HOSTS),
	  targetHostgroupId(ALL_HOST_GROUPS),
	  filterDataOfDefunctServers(true)
	{
	}

	Impl &operator=(const Impl &rhs)
	{
		targetServerId             = rhs.targetServerId;
		targetHostId               = rhs.targetHostId;
		targetHostgroupId          = rhs.targetHostgroupId;
		filterDataOfDefunctServers = rhs.filterDataOfDefunctServers;
		return *this;
	}
};

// TODO: Use a more smart way
static DBTermCodec _dbTermCodec;
const DBTermCodec *HostResourceQueryOption::Impl::dbTermCodec =
  &_dbTermCodec;
// Use this if the backend DB is SQLite3:
//  DBAgentSQLite3::getDBTermCodecStatic();

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HostResourceQueryOption::HostResourceQueryOption(
  const Synapse &synapse, const UserIdType &userId)
: DataQueryOption(userId),
  m_impl(new Impl(synapse))
{
}

HostResourceQueryOption::HostResourceQueryOption(
  const Synapse &synapse, DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext),
  m_impl(new Impl(synapse))
{
}

HostResourceQueryOption::HostResourceQueryOption(
  const HostResourceQueryOption &src)
: DataQueryOption(src),
  m_impl(new Impl(src.m_impl->synapse))
{
	*m_impl = *src.m_impl;
}

HostResourceQueryOption::~HostResourceQueryOption()
{
}

const char *HostResourceQueryOption::getPrimaryTableName(void) const
{
	return m_impl->synapse.tableProfile.name;
}

string HostResourceQueryOption::getCondition(void) const
{
	const DBTermCodec *dbTermCodec = Impl::dbTermCodec;
	string condition;
	if (getFilterForDataOfDefunctServers()) {
		addCondition(
		  condition,
		  makeConditionServer(
		    getDataQueryContext().getValidServerIdSet(),
		    getServerIdColumnName())
		);
	}

	UserIdType userId = getUserId();

	// TODO: consider if we cau use isHostgroupEnumerationInCondition()
	if (userId == USER_ID_SYSTEM || has(OPPRVLG_GET_ALL_SERVER)) {
		if (m_impl->targetServerId != ALL_SERVERS) {
			addCondition(condition,
			  StringUtils::sprintf(
				"%s=%s",
				getServerIdColumnName().c_str(),
				dbTermCodec->enc(m_impl->targetServerId).c_str())
			);
		}
		if (m_impl->targetHostId != ALL_HOSTS) {
			addCondition(condition,
			  StringUtils::sprintf(
				"%s=%s",
				getHostIdColumnName().c_str(),
				dbTermCodec->enc(m_impl->targetHostId).c_str())
			);
		}
		if (m_impl->targetHostgroupId != ALL_HOST_GROUPS) {
			addCondition(condition,
			  StringUtils::sprintf(
				"%s=%s",
				getHostgroupIdColumnName().c_str(),
				dbTermCodec->enc(m_impl->targetHostgroupId).c_str())
			);
		}
		return condition;
	}

	if (userId == INVALID_USER_ID) {
		MLPL_DBG("INVALID_USER_ID\n");
		return DBHatohol::getAlwaysFalseCondition();
	}

	// If the subclass doesn't have a valid hostIdColumnIdx,
	// getHostIdColumnName() throws an exception. In that case,
	// targetHostId shall be ALL_HOSTS.
	const string hostIdColumnName =
	  (m_impl->targetHostId != ALL_HOSTS) ?  getHostIdColumnName() : "";

	const ServerHostGrpSetMap &srvHostGrpSetMap =
	  getDataQueryContext().getServerHostGrpSetMap();
	addCondition(condition,
	             makeCondition(srvHostGrpSetMap,
	                           getServerIdColumnName(),
	                           getHostgroupIdColumnName(),
	                           hostIdColumnName,
	                           m_impl->targetServerId,
	                           m_impl->targetHostgroupId,
	                           m_impl->targetHostId));
	return condition;
}

string HostResourceQueryOption::getFromClause(void) const
{
	if (isHostgroupUsed())
		return getFromClauseWithHostgroup();
	else
		return getFromClauseForOneTable();
}

string HostResourceQueryOption::getJoinClause(void) const
{
	if (!isHostgroupUsed())
		return string();

	const Synapse &synapse = m_impl->synapse;
	const ColumnDef *hgrpColumnDefs =
	  synapse.hostgroupMapTableProfile.columnDefs;

	return StringUtils::sprintf(
	  "INNER JOIN %s ON ((%s=%s.%s) AND (%s=%s.%s))",
	  synapse.hostgroupMapTableProfile.name,

	  getServerIdColumnName().c_str(),
	  synapse.hostgroupMapTableProfile.name,
	  hgrpColumnDefs[synapse.hostgroupMapServerIdColumnIdx].columnName,

	  getHostIdColumnName().c_str(),
	  synapse.hostgroupMapTableProfile.name,
	  hgrpColumnDefs[synapse.hostgroupMapHostIdColumnIdx].columnName);
}

bool HostResourceQueryOption::isHostgroupUsed(void) const
{
	const Synapse &synapse = m_impl->synapse;
	if (!synapse.needToJoinHostgroup)
		return false;
	if (isHostgroupEnumerationInCondition())
		return true;
	return m_impl->targetHostgroupId != ALL_HOST_GROUPS;
}

string HostResourceQueryOption::getColumnName(const size_t &idx) const
{
	return getColumnNameCommon(m_impl->synapse.tableProfile, idx);
}

string HostResourceQueryOption::getHostgroupColumnName(const size_t &idx) const
{
	return getColumnNameCommon(m_impl->synapse.hostgroupMapTableProfile,
	                           idx);
}

ServerIdType HostResourceQueryOption::getTargetServerId(void) const
{
	return m_impl->targetServerId;
}

void HostResourceQueryOption::setTargetServerId(const ServerIdType &targetServerId)
{
	m_impl->targetServerId = targetServerId;
}

HostIdType HostResourceQueryOption::getTargetHostId(void) const
{
	return m_impl->targetHostId;
}

void HostResourceQueryOption::setTargetHostId(HostIdType targetHostId)
{
	m_impl->targetHostId = targetHostId;
}

HostgroupIdType HostResourceQueryOption::getTargetHostgroupId(void) const
{
	return m_impl->targetHostgroupId;
}

void HostResourceQueryOption::setTargetHostgroupId(
  HostgroupIdType targetHostgroupId)
{
	m_impl->targetHostgroupId = targetHostgroupId;
}

void HostResourceQueryOption::setFilterForDataOfDefunctServers(
  const bool &enable)
{
	m_impl->filterDataOfDefunctServers = enable;
}

const bool &HostResourceQueryOption::getFilterForDataOfDefunctServers(void) const
{
	return m_impl->filterDataOfDefunctServers;
}

const DBTermCodec *HostResourceQueryOption::getDBTermCodec(void) const
{
	return m_impl->dbTermCodec;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
string HostResourceQueryOption::getServerIdColumnName(void) const
{
	return getColumnName(m_impl->synapse.serverIdColumnIdx);
}

string HostResourceQueryOption::getHostgroupIdColumnName(void) const
{
	const size_t &idx = m_impl->synapse.hostgroupMapGroupIdColumnIdx;
	return getHostgroupColumnName(idx);
}

string HostResourceQueryOption::getHostIdColumnName(void) const
{
	return getColumnNameCommon(m_impl->synapse.hostTableProfile,
	                           m_impl->synapse.hostIdColumnIdx);
}

string HostResourceQueryOption::makeConditionHostgroup(
  const HostgroupIdSet &hostgroupIdSet, const string &hostgroupIdColumnName)
{
	string hostGrps;
	HostgroupIdSetConstIterator it = hostgroupIdSet.begin();
	size_t commaCnt = hostgroupIdSet.size() - 1;
	for (; it != hostgroupIdSet.end(); ++it, commaCnt--) {
		const uint64_t hostgroupId = *it;
		if (hostgroupId == ALL_HOST_GROUPS)
			return "";
		hostGrps += StringUtils::sprintf("%" PRIu64, hostgroupId);
		if (commaCnt)
			hostGrps += ",";
	}
	string cond = StringUtils::sprintf(
	  "%s IN (%s)", hostgroupIdColumnName.c_str(), hostGrps.c_str());
	return cond;
}

string HostResourceQueryOption::makeConditionServer(
  const ServerIdSet &serverIdSet, const std::string &serverIdColumnName)
{
	if (serverIdSet.empty())
		return DBHatohol::getAlwaysFalseCondition();

	string condition = StringUtils::sprintf(
	  "%s IN (", serverIdColumnName.c_str());

	ServerIdSetConstIterator serverId = serverIdSet.begin();
	bool first = true;
	const DBTermCodec *dbTermCodec = Impl::dbTermCodec;
	for (; serverId != serverIdSet.end(); ++serverId) {
		if (first)
			first = false;
		else
			condition += ",";
		condition += StringUtils::sprintf(
		  "%s", dbTermCodec->enc(*serverId).c_str());
	}
	condition += ")";
	return condition;
}

string HostResourceQueryOption::makeConditionServer(
  const ServerIdType &serverId, const HostgroupIdSet &hostgroupIdSet,
  const string &serverIdColumnName, const string &hostgroupIdColumnName,
  const HostgroupIdType &hostgroupId)
{
	const DBTermCodec *dbTermCodec = Impl::dbTermCodec;
	string condition;
	condition = StringUtils::sprintf(
	  "%s=%s", serverIdColumnName.c_str(),
	  dbTermCodec->enc(serverId).c_str());

	string conditionHostgroup;
	if (hostgroupId == ALL_HOST_GROUPS) {
		conditionHostgroup =
		  makeConditionHostgroup(hostgroupIdSet, hostgroupIdColumnName);
	} else {
		conditionHostgroup = StringUtils::sprintf(
		  "%s=%s", hostgroupIdColumnName.c_str(),
		  dbTermCodec->enc(hostgroupId).c_str());
	}
	if (!conditionHostgroup.empty()) {
		return StringUtils::sprintf("(%s AND %s)",
					    condition.c_str(),
					    conditionHostgroup.c_str());
	} else {
		return condition;
	}
}

string HostResourceQueryOption::makeCondition(
  const ServerHostGrpSetMap &srvHostGrpSetMap,
  const string &serverIdColumnName,
  const string &hostgroupIdColumnName,
  const string &hostIdColumnName,
  ServerIdType targetServerId,
  HostgroupIdType targetHostgroupId,
  HostIdType targetHostId)
{
	// TODO: consider if we use isHostgroupEnumerationInCondition()
	string condition;

	size_t numServers = srvHostGrpSetMap.size();
	if (numServers == 0) {
		MLPL_DBG("No allowed server\n");
		return DBHatohol::getAlwaysFalseCondition();
	}

	if (targetServerId != ALL_SERVERS &&
	    srvHostGrpSetMap.find(targetServerId) == srvHostGrpSetMap.end())
	{
		return DBHatohol::getAlwaysFalseCondition();
	}

	numServers = 0;
	ServerHostGrpSetMapConstIterator it = srvHostGrpSetMap.begin();
	for (; it != srvHostGrpSetMap.end(); ++it) {
		const ServerIdType &serverId = it->first;

		if (targetServerId != ALL_SERVERS && targetServerId != serverId)
			continue;

		if (serverId == ALL_SERVERS)
			return "";

		string conditionServer = makeConditionServer(
					   serverId, it->second,
					   serverIdColumnName,
					   hostgroupIdColumnName,
					   targetHostgroupId);
		addCondition(condition, conditionServer, ADD_TYPE_OR);
		++numServers;
	}

	if (targetHostId != ALL_HOSTS) {
		const DBTermCodec *dbTermCodec = Impl::dbTermCodec;
		return StringUtils::sprintf(
		  "((%s) AND %s=%s)",
		  condition.c_str(), hostIdColumnName.c_str(),
		  dbTermCodec->enc(targetHostId).c_str());
	}

	if (numServers == 1)
		return condition;
	return StringUtils::sprintf("(%s)", condition.c_str());
}

string HostResourceQueryOption::getFromClauseForOneTable(void) const
{
	return getPrimaryTableName();
}

string HostResourceQueryOption::getFromClauseWithHostgroup(void) const
{
	const Synapse &synapse = m_impl->synapse;

	return StringUtils::sprintf(
	  "%s %s",
	  synapse.tableProfile.name,
	  getJoinClause().c_str());
}

string HostResourceQueryOption::getColumnNameCommon(
  const DBAgent::TableProfile &tableProfile, const size_t &idx) const
{
	HATOHOL_ASSERT(idx < tableProfile.numColumns,
	               "idx: %zd, numColumns: %zd, table: %s",
	               idx, tableProfile.numColumns, tableProfile.name);
	string name;
	if (getTableNameAlways() || isHostgroupUsed()) {
		name = tableProfile.getFullColumnName(idx);
	} else {
		const ColumnDef *columnDefs = tableProfile.columnDefs;
		name = columnDefs[idx].columnName;
	}
	return name;
}

bool HostResourceQueryOption::isHostgroupEnumerationInCondition(void) const
{
	const UserIdType &userId = getUserId();
	if (userId == USER_ID_SYSTEM || has(OPPRVLG_GET_ALL_SERVER))
		return false;
	const ServerHostGrpSetMap &srvHostGrpSetMap =
	  getDataQueryContext().getServerHostGrpSetMap();
	ServerHostGrpSetMapConstIterator it = srvHostGrpSetMap.begin();
	for (; it != srvHostGrpSetMap.end(); ++it) {
		const ServerIdType &serverId = it->first;
		if (serverId == ALL_SERVERS)
			continue;
		const HostgroupIdSet &hostgrpIdSet = it->second;
		if (hostgrpIdSet.find(ALL_HOST_GROUPS) == hostgrpIdSet.end())
			return true;
	}
	return false;
}
