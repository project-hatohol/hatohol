/*
 * Copyright (C) 2013-2015 Project Hatohol
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
#include "DBTermCStringProvider.h"
#include "DBHatohol.h"

using namespace std;
using namespace mlpl;

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
  const size_t &_hostgroupMapGroupIdColumnIdx,
  const size_t &_globalHostIdColumnIdx,
  const size_t &_hostgroupMapGlobalHostIdColumnIdx)
: tableProfile(_tableProfile),
  selfIdColumnIdx(_selfIdColumnIdx),
  serverIdColumnIdx(_serverIdColumnIdx),
  hostTableProfile(_hostTableProfile),
  hostIdColumnIdx(_hostIdColumnIdx),
  needToJoinHostgroup(_needToJoinHostgroup),
  hostgroupMapTableProfile(_hostgroupMapTableProfile),
  hostgroupMapServerIdColumnIdx(_hostgroupMapServerIdColumnIdx),
  hostgroupMapHostIdColumnIdx(_hostgroupMapHostIdColumnIdx),
  hostgroupMapGroupIdColumnIdx(_hostgroupMapGroupIdColumnIdx),
  globalHostIdColumnIdx(_globalHostIdColumnIdx),
  hostgroupMapGlobalHostIdColumnIdx(_hostgroupMapGlobalHostIdColumnIdx)
{
}

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct HostResourceQueryOption::Impl {
	const Synapse  &synapse;
	ServerIdType    targetServerId;
	LocalHostIdType targetHostId;
	HostgroupIdType targetHostgroupId;
	bool            excludeDefunctServers;
	ServerIdSet filterServerIdSet;
	bool excludeServerIdSet;
	ServerHostGrpSetMap filterServerHostgroupSetMap;
	bool excludeServerHostgroupSetMap;
	ServerHostSetMap filterServerHostSetMap;
	bool excludeServerHostSetMap;

	// For unit tests
	const ServerIdSet *validServerIdSet;
	const ServerHostGrpSetMap *allowedServersAndHostgroups;

	Impl(const Synapse &_synapse)
	: synapse(_synapse),
	  targetServerId(ALL_SERVERS),
	  targetHostId(ALL_LOCAL_HOSTS),
	  targetHostgroupId(ALL_HOST_GROUPS),
	  excludeDefunctServers(true),
	  excludeServerIdSet(false),
	  excludeServerHostgroupSetMap(false),
	  excludeServerHostSetMap(false),
	  validServerIdSet(NULL),
	  allowedServersAndHostgroups(NULL)
	{
	}

	Impl &operator=(const Impl &rhs)
	{
		targetServerId        = rhs.targetServerId;
		targetHostId          = rhs.targetHostId;
		targetHostgroupId     = rhs.targetHostgroupId;
		excludeDefunctServers = rhs.excludeDefunctServers;
		return *this;
	}
};

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
	string condition;

	// Select only alive servers
	if (getExcludeDefunctServers()) {
		string validServersCondition(
		  makeConditionServer(getValidServerIdSet(),
				      getServerIdColumnName()));
		addCondition(condition, validServersCondition);
	}

	// Select only allowed servers and hostgroups
	if (!has(OPPRVLG_GET_ALL_SERVER)) {
		string allowedHostsCondition(makeConditionAllowedHosts());

		if (DBHatohol::isAlwaysFalseCondition(allowedHostsCondition))
			return allowedHostsCondition;

		addCondition(condition, allowedHostsCondition);
	}

	// Select servers, hostgroups and hosts sepcified by the caller
	addCondition(condition, makeConditionTargetIds());
	addCondition(condition, makeConditionFilter());

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
	struct {
		bool operator()(const size_t &index)
		{
			return index != INVALID_COLUMN_IDX;
		}
	} valid;

	if (!isHostgroupUsed())
		return string();

	const Synapse &synapse = m_impl->synapse;
	if (valid(synapse.globalHostIdColumnIdx) &&
	    valid(synapse.hostgroupMapGlobalHostIdColumnIdx)) {
		return getJoinClauseWithGlobalHostId();
	}

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
	return
	  m_impl->targetHostgroupId != ALL_HOST_GROUPS ||
	  !m_impl->filterServerHostgroupSetMap.empty();
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

LocalHostIdType HostResourceQueryOption::getTargetHostId(void) const
{
	return m_impl->targetHostId;
}

void HostResourceQueryOption::setTargetHostId(
  const LocalHostIdType &targetHostId)
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

void HostResourceQueryOption::setExcludeDefunctServers(
  const bool &enable)
{
	m_impl->excludeDefunctServers = enable;
}

const bool &HostResourceQueryOption::getExcludeDefunctServers(void) const
{
	return m_impl->excludeDefunctServers;
}

void HostResourceQueryOption::setFilterServerIds(
  const ServerIdSet &serverIds, const bool exclude)
{
	m_impl->filterServerIdSet = serverIds;
	m_impl->excludeServerIdSet = exclude;
}

void HostResourceQueryOption::getFilterServerIds(
  ServerIdSet &serverIds, bool &exclude)
{
	serverIds = m_impl->filterServerIdSet;
	exclude = m_impl->excludeServerIdSet;
}

void HostResourceQueryOption::setFilterHostgroupIds(
  const ServerHostGrpSetMap &hostgroupIds, const bool exclude)
{
	m_impl->filterServerHostgroupSetMap = hostgroupIds;
	m_impl->excludeServerHostgroupSetMap = exclude;
}

void HostResourceQueryOption::getFilterHostgroupIds(
  ServerHostGrpSetMap &hostgroupIds, bool &exclude)
{
	hostgroupIds = m_impl->filterServerHostgroupSetMap;
	exclude = m_impl->excludeServerHostgroupSetMap;
}

void HostResourceQueryOption::setFilterHostIds(
  const ServerHostSetMap &hostIds, const bool exclude)
{
	m_impl->filterServerHostSetMap = hostIds;
	m_impl->excludeServerHostSetMap = exclude;
}

void HostResourceQueryOption::getFilterHostIds(
  ServerHostSetMap &hostIds, bool &exclude)
{
	hostIds = m_impl->filterServerHostSetMap;
	exclude = m_impl->excludeServerHostSetMap;
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

bool HostResourceQueryOption::isAllowedServer(
  const ServerIdType &targetServerId) const
{
	if (has(OPPRVLG_GET_ALL_SERVER))
		return true;
	const ServerHostGrpSetMap &allowedServersAndHostgroups =
	  getAllowedServersAndHostgroups();
	auto endIt = allowedServersAndHostgroups.end();
	return (allowedServersAndHostgroups.find(ALL_SERVERS) != endIt) ||
	       (allowedServersAndHostgroups.find(targetServerId) != endIt);
}

bool HostResourceQueryOption::isAllowedHostgroup(
  const ServerIdType &targetServerId,
  const HostgroupIdType &targetHostgroupId) const
{
	if (has(OPPRVLG_GET_ALL_SERVER))
		return true;
	const ServerHostGrpSetMap &allowedServersAndHostgroups =
	  getAllowedServersAndHostgroups();
	auto endServerIt(allowedServersAndHostgroups.end());
	if (allowedServersAndHostgroups.find(ALL_SERVERS) != endServerIt)
		return true;
	auto serverIt(allowedServersAndHostgroups.find(targetServerId));
	if (serverIt != allowedServersAndHostgroups.end())
		return false;
	const HostgroupIdSet &hostgroupSet = serverIt->second;
	return hostgroupSet.find(targetHostgroupId) != hostgroupSet.end();
}

string HostResourceQueryOption::makeConditionTargetIds(void) const
{
	string condition;
	DBTermCStringProvider rhs(*getDBTermCodec());

	if (m_impl->targetServerId != ALL_SERVERS) {
		addCondition(condition,
		  StringUtils::sprintf(
		    "%s=%s",
		    getServerIdColumnName().c_str(),
		    rhs(m_impl->targetServerId)));
	}
	if (m_impl->targetHostId != ALL_LOCAL_HOSTS) {
		addCondition(condition,
		  StringUtils::sprintf(
		    "%s=%s",
		    getHostIdColumnName().c_str(),
		    rhs(m_impl->targetHostId)));
	}
	if (m_impl->targetHostgroupId != ALL_HOST_GROUPS) {
		addCondition(condition,
		  StringUtils::sprintf(
		    "%s=%s",
		    getHostgroupIdColumnName().c_str(),
		    rhs(m_impl->targetHostgroupId)));
	}

	if (condition.empty())
		return condition;
	return StringUtils::sprintf("(%s)", condition.c_str());
}

string HostResourceQueryOption::makeConditionHostgroup(
  const HostgroupIdSet &hostgroupIdSet, const string &hostgroupIdColumnName)
  const
{
	DBTermCStringProvider rhs(*getDBTermCodec());
	string hostGrps;
	HostgroupIdSetConstIterator it = hostgroupIdSet.begin();
	size_t commaCnt = hostgroupIdSet.size() - 1;
	for (; it != hostgroupIdSet.end(); ++it, commaCnt--) {
		const HostgroupIdType &hostgroupId = *it;
		if (hostgroupId == ALL_HOST_GROUPS)
			return "";
		hostGrps += rhs(hostgroupId);
		if (commaCnt)
			hostGrps += ",";
	}
	string cond = StringUtils::sprintf(
	  "%s IN (%s)", hostgroupIdColumnName.c_str(), hostGrps.c_str());
	return cond;
}

string HostResourceQueryOption::makeConditionServer(
  const ServerIdSet &serverIdSet, const std::string &serverIdColumnName) const
{
	if (serverIdSet.empty())
		return DBHatohol::getAlwaysFalseCondition();

	string condition = StringUtils::sprintf(
	  "%s IN (", serverIdColumnName.c_str());

	ServerIdSetConstIterator serverId = serverIdSet.begin();
	bool first = true;
	DBTermCStringProvider rhs(*getDBTermCodec());
	for (; serverId != serverIdSet.end(); ++serverId) {
		if (first)
			first = false;
		else
			condition += ",";
		condition += rhs(*serverId);
	}
	condition += ")";
	return condition;
}

string HostResourceQueryOption::makeConditionServer(
  const ServerIdType &serverId, const HostgroupIdSet &hostgroupIdSet,
  const string &serverIdColumnName, const string &hostgroupIdColumnName,
  const HostgroupIdType &hostgroupId) const
{
	DBTermCStringProvider rhs(*getDBTermCodec());
	string condition;
	condition = StringUtils::sprintf(
	  "%s=%s", serverIdColumnName.c_str(), rhs(serverId));

	string conditionHostgroup;
	if (hostgroupId == ALL_HOST_GROUPS) {
		conditionHostgroup =
		  makeConditionHostgroup(hostgroupIdSet, hostgroupIdColumnName);
	} else {
		conditionHostgroup = StringUtils::sprintf(
		  "%s=%s", hostgroupIdColumnName.c_str(),
		  rhs(hostgroupId.c_str()));
	}
	if (!conditionHostgroup.empty()) {
		return StringUtils::sprintf("(%s AND %s)",
					    condition.c_str(),
					    conditionHostgroup.c_str());
	} else {
		return condition;
	}
	return condition;
}

string HostResourceQueryOption::makeConditionAllowedHosts(void) const
{
	const ServerHostGrpSetMap &allowedServersAndHostgroups =
	  getAllowedServersAndHostgroups();

	const string &serverIdColumnName = getServerIdColumnName();
	const string &hostgroupIdColumnName = getHostgroupIdColumnName();

	// If the subclass doesn't have a valid hostIdColumnIdx
	// (HostgroupsQueryOption doesn't have it), getHostIdColumnName()
	// throws an exception. In that case, targetHostId shall be ALL_HOSTS
	// and hostIdColumnName shall not be needed. So we use dummy
	// hostIdColumnName in this case.
	const string hostIdColumnName =
	  (m_impl->targetHostId != ALL_LOCAL_HOSTS) ?
	    getHostIdColumnName() : "";

	// TODO: consider if we use isHostgroupEnumerationInCondition()
	string condition;
	const ServerIdType &targetServerId = m_impl->targetServerId;
	const HostgroupIdType &targetHostgroupId = m_impl->targetHostgroupId;

	size_t numServers = allowedServersAndHostgroups.size();
	if (numServers == 0) {
		MLPL_DBG("No allowed server\n");
		return DBHatohol::getAlwaysFalseCondition();
	}

	if (targetServerId != ALL_SERVERS && !isAllowedServer(targetServerId)) {
		return DBHatohol::getAlwaysFalseCondition();
	}

	numServers = 0;
	ServerHostGrpSetMapConstIterator it = allowedServersAndHostgroups.begin();
	for (; it != allowedServersAndHostgroups.end(); ++it) {
		const ServerIdType &serverId = it->first;

		if (targetServerId != ALL_SERVERS && targetServerId != serverId) {
			// Omit it because it isn't the target server.
			continue;
		}

		if (serverId == ALL_SERVERS) {
			// All servers are allowed
			return "";
		}

		string conditionServer = makeConditionServer(
					   serverId, it->second,
					   serverIdColumnName,
					   hostgroupIdColumnName,
					   targetHostgroupId);
		addCondition(condition, conditionServer, ADD_TYPE_OR);
		++numServers;
	}

	if (numServers == 1)
		return condition;
	return StringUtils::sprintf("(%s)", condition.c_str());
}

string HostResourceQueryOption::makeConditionServersFilter(void) const
{
	if (m_impl->filterServerIdSet.empty())
		return string();

	DBTermCStringProvider rhs(*getDBTermCodec());
	string condition;
	if (m_impl->excludeServerIdSet) {
		for (auto &serverId: m_impl->filterServerIdSet) {
			if (!isAllowedServer(serverId))
				continue;
			addCondition(condition,
				     StringUtils::sprintf(
				       "%s<>%s",
				       getServerIdColumnName().c_str(),
				       rhs(serverId)));
		}
	} else {
		for (auto &serverId: m_impl->filterServerIdSet) {
			if (!isAllowedServer(serverId))
				continue;
			addCondition(condition,
				     StringUtils::sprintf(
				       "%s=%s",
				       getServerIdColumnName().c_str(),
				       rhs(serverId)),
				     ADD_TYPE_OR);
		}
	}

	if (m_impl->filterServerIdSet.size() > 1)
		return StringUtils::sprintf("(%s)", condition.c_str());
	else
		return condition;
}

string HostResourceQueryOption::makeConditionHostgroupsFilter(void) const
{
	if (m_impl->filterServerHostgroupSetMap.empty())
		return string();

	DBTermCStringProvider rhs(*getDBTermCodec());
	string condition;
	string serverIdColumnName = getServerIdColumnName();
	string hostgroupIdColumnName = getHostgroupIdColumnName();

	if (m_impl->excludeServerHostgroupSetMap) {
		for (auto &pair: m_impl->filterServerHostgroupSetMap) {
			if (!isAllowedServer(pair.first))
				continue;
			for (auto &hostgroupId: pair.second) {
				if (!isAllowedHostgroup(pair.first, hostgroupId))
					continue;
				addCondition(condition,
					     StringUtils::sprintf(
					       "NOT (%s=%s AND %s=%s)",
					       serverIdColumnName.c_str(),
					       rhs(pair.first),
					       hostgroupIdColumnName.c_str(),
					       rhs(hostgroupId)));
			}
		}
	} else {
		for (auto &pair: m_impl->filterServerHostgroupSetMap) {
			if (!isAllowedServer(pair.first))
				continue;
			for (auto &hostgroupId: pair.second) {
				if (!isAllowedHostgroup(pair.first, hostgroupId))
					continue;
				addCondition(condition,
					     StringUtils::sprintf(
					       "(%s=%s AND %s=%s)",
					       serverIdColumnName.c_str(),
					       rhs(pair.first),
					       hostgroupIdColumnName.c_str(),
					       rhs(hostgroupId)),
					     ADD_TYPE_OR);
			}
		}
	}

	if (m_impl->filterServerHostgroupSetMap.size() > 1)
		return StringUtils::sprintf("(%s)", condition.c_str());
	else
		return condition;
}

string HostResourceQueryOption::makeConditionHostsFilter(void) const
{
	if (m_impl->filterServerHostSetMap.empty())
		return string();

	DBTermCStringProvider rhs(*getDBTermCodec());
	string condition;
	string serverIdColumnName = getServerIdColumnName();
	string hostIdColumnName = getHostIdColumnName();

	if (m_impl->excludeServerHostSetMap) {
		for (auto &pair: m_impl->filterServerHostSetMap) {
			if (!isAllowedServer(pair.first))
				continue;
			for (auto &hostId: pair.second) {
				addCondition(condition,
					     StringUtils::sprintf(
					       "NOT (%s=%s AND %s=%s)",
					       serverIdColumnName.c_str(),
					       rhs(pair.first),
					       hostIdColumnName.c_str(),
					       rhs(hostId)));
			}
		}
	} else {
		for (auto &pair: m_impl->filterServerHostSetMap) {
			if (!isAllowedServer(pair.first))
				continue;
			for (auto &hostId: pair.second) {
				addCondition(condition,
					     StringUtils::sprintf(
					       "(%s=%s AND %s=%s)",
					       serverIdColumnName.c_str(),
					       rhs(pair.first),
					       hostIdColumnName.c_str(),
					       rhs(hostId)),
					     ADD_TYPE_OR);
			}
		}
	}

	if (m_impl->filterServerHostSetMap.size() > 1)
		return StringUtils::sprintf("(%s)", condition.c_str());
	else
		return condition;
}

string HostResourceQueryOption::makeConditionFilter(void) const
{
	string condition, nextCondition;
	size_t numFilters = 0;

	AddConditionType addType = m_impl->excludeServerIdSet ?
		ADD_TYPE_AND : ADD_TYPE_OR;
	nextCondition = makeConditionServersFilter();
	if (!nextCondition.empty())
		++numFilters;
	addCondition(condition, nextCondition, addType);

	addType = m_impl->excludeServerHostgroupSetMap ?
		ADD_TYPE_AND : ADD_TYPE_OR;
	nextCondition = makeConditionHostgroupsFilter();
	if (!nextCondition.empty())
		++numFilters;
	addCondition(condition, nextCondition, addType);

	addType = m_impl->excludeServerHostSetMap ?
		ADD_TYPE_AND : ADD_TYPE_OR;
	nextCondition = makeConditionHostsFilter();
	if (!nextCondition.empty())
		++numFilters;
	addCondition(condition, nextCondition, addType);

	if (numFilters > 1)
		return StringUtils::sprintf("(%s)", condition.c_str());
	else
		return condition;
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
	if (has(OPPRVLG_GET_ALL_SERVER))
		return false;
	const ServerHostGrpSetMap &allowedServersAndHostgroups =
	  getAllowedServersAndHostgroups();
	ServerHostGrpSetMapConstIterator it
	  = allowedServersAndHostgroups.begin();
	for (; it != allowedServersAndHostgroups.end(); ++it) {
		const ServerIdType &serverId = it->first;
		if (serverId == ALL_SERVERS)
			continue;
		const HostgroupIdSet &hostgrpIdSet = it->second;
		if (hostgrpIdSet.find(ALL_HOST_GROUPS) == hostgrpIdSet.end())
			return true;
	}
	return false;
}

string HostResourceQueryOption::getJoinClauseWithGlobalHostId(void) const
{
	const Synapse &synapse = m_impl->synapse;
	return StringUtils::sprintf(
	  "INNER JOIN %s ON %s=%s",
	  synapse.hostgroupMapTableProfile.name,
	  synapse.tableProfile.getFullColumnName(
	    synapse.globalHostIdColumnIdx).c_str(),
	  synapse.hostgroupMapTableProfile.getFullColumnName(
	    synapse.hostgroupMapGlobalHostIdColumnIdx).c_str());
}

const ServerIdSet &HostResourceQueryOption::getValidServerIdSet(void) const
{
	if (m_impl->validServerIdSet)
		return *m_impl->validServerIdSet;
	return getDataQueryContext().getValidServerIdSet();
}

const ServerHostGrpSetMap &
HostResourceQueryOption::getAllowedServersAndHostgroups(void) const
{
	if (m_impl->allowedServersAndHostgroups)
		return *m_impl->allowedServersAndHostgroups;
	return getDataQueryContext().getServerHostGrpSetMap();
}

// For test use only
void HostResourceQueryOption::setValidServerIdSet(const ServerIdSet *set)
{
	m_impl->validServerIdSet = set;
}

void HostResourceQueryOption::setAllowedServersAndHostgroups (
  const ServerHostGrpSetMap *map)
{
	m_impl->allowedServersAndHostgroups = map;
}
