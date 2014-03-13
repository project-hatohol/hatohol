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

#include "Params.h"
#include "HostResourceQueryOption.h"
#include "DBClientHatohol.h"
using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// PrivateContext
// ---------------------------------------------------------------------------
struct HostResourceQueryOption::PrivateContext {
	const char     *primaryTableName;
	string          serverIdColumnName;
	string          hostGroupIdColumnName;
	string          hostIdColumnName;
	ServerIdType    targetServerId;
	HostIdType      targetHostId;
	HostgroupIdType targetHostgroupId;
	bool            filterDataOfDefunctServers;

	PrivateContext(const char *_primaryTableName)
	: primaryTableName(_primaryTableName),
	  serverIdColumnName("server_id"),
	  hostGroupIdColumnName("host_group_id"),
	  hostIdColumnName("host_id"),
	  targetServerId(ALL_SERVERS),
	  targetHostId(ALL_HOSTS),
	  targetHostgroupId(ALL_HOST_GROUPS),
	  filterDataOfDefunctServers(true)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HostResourceQueryOption::HostResourceQueryOption(
  const char *primaryTableName, const UserIdType &userId)
: DataQueryOption(userId)
{
	m_ctx = new PrivateContext(primaryTableName);
}

HostResourceQueryOption::HostResourceQueryOption(
  const char *primaryTableName, DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext)
{
	m_ctx = new PrivateContext(primaryTableName);
}

HostResourceQueryOption::HostResourceQueryOption(
  const HostResourceQueryOption &src)
: DataQueryOption(src)
{
	m_ctx = new PrivateContext(src.m_ctx->primaryTableName);
	*m_ctx = *src.m_ctx;
}

HostResourceQueryOption::~HostResourceQueryOption()
{
	if (m_ctx)
		delete m_ctx;
}

string HostResourceQueryOption::getCondition(const string &tableAlias) const
{
	string condition;
	string hostgroupTableAlias;
	if (!tableAlias.empty())
		hostgroupTableAlias = DBClientHatohol::TABLE_NAME_MAP_HOSTS_HOSTGROUPS;
	if (getFilterForDataOfDefunctServers()) {
		addCondition(
		  condition,
		  makeConditionServer(
		    getDataQueryContext().getValidServerIdSet(),
		    getServerIdColumnName(tableAlias))
		);
	}

	UserIdType userId = getUserId();

	if (userId == USER_ID_SYSTEM || has(OPPRVLG_GET_ALL_SERVER)) {
		if (m_ctx->targetServerId != ALL_SERVERS) {
			addCondition(condition,
			  StringUtils::sprintf(
				"%s=%"FMT_SERVER_ID,
				getServerIdColumnName(tableAlias).c_str(),
				m_ctx->targetServerId));
		}
		if (m_ctx->targetHostId != ALL_HOSTS) {
			addCondition(condition,
			  StringUtils::sprintf(
				"%s=%"FMT_HOST_ID,
				getHostIdColumnName(tableAlias).c_str(),
				m_ctx->targetHostId));
		}
		if (m_ctx->targetHostgroupId != ALL_HOST_GROUPS) {
			addCondition(condition,
			  StringUtils::sprintf(
				"%s=%"FMT_HOST_GROUP_ID,
				getHostgroupIdColumnName(
				  hostgroupTableAlias).c_str(),
				m_ctx->targetHostgroupId));
		}
		return condition;
	}

	if (userId == INVALID_USER_ID) {
		MLPL_DBG("INVALID_USER_ID\n");
		return DBClientHatohol::getAlwaysFalseCondition();
	}

	const ServerHostGrpSetMap &srvHostGrpSetMap =
	  getDataQueryContext().getServerHostGrpSetMap();
	addCondition(condition,
	             makeCondition(srvHostGrpSetMap,
	                           getServerIdColumnName(tableAlias),
	                           getHostgroupIdColumnName(
	                             hostgroupTableAlias),
	                           getHostIdColumnName(tableAlias),
	                           m_ctx->targetServerId,
	                           m_ctx->targetHostgroupId,
	                           m_ctx->targetHostId));
	return condition;
}


string HostResourceQueryOption::generateFromSection(void)
{
	if (m_ctx->targetHostgroupId == ALL_HOST_GROUPS)
		return getFromSectionForOneTable();
	else
		return getFromSectionWithHostgroup();
}

ServerIdType HostResourceQueryOption::getTargetServerId(void) const
{
	return m_ctx->targetServerId;
}

void HostResourceQueryOption::setTargetServerId(const ServerIdType &targetServerId)
{
	m_ctx->targetServerId = targetServerId;
}

HostIdType HostResourceQueryOption::getTargetHostId(void) const
{
	return m_ctx->targetHostId;
}

void HostResourceQueryOption::setTargetHostId(HostIdType targetHostId)
{
	m_ctx->targetHostId = targetHostId;
}

HostgroupIdType HostResourceQueryOption::getTargetHostgroupId(void) const
{
	return m_ctx->targetHostgroupId;
}

void HostResourceQueryOption::setTargetHostgroupId(
  HostgroupIdType targetHostgroupId)
{
	m_ctx->targetHostgroupId = targetHostgroupId;
}

void HostResourceQueryOption::setFilterForDataOfDefunctServers(
  const bool &enable)
{
	m_ctx->filterDataOfDefunctServers = enable;
}

const bool &HostResourceQueryOption::getFilterForDataOfDefunctServers(void) const
{
	return m_ctx->filterDataOfDefunctServers;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HostResourceQueryOption::setServerIdColumnName(
  const std::string &name) const
{
	m_ctx->serverIdColumnName = name;
}

string HostResourceQueryOption::getServerIdColumnName(
  const std::string &tableAlias) const
{
	if (tableAlias.empty())
		return m_ctx->serverIdColumnName;

	return StringUtils::sprintf("%s.%s",
				    tableAlias.c_str(),
				    m_ctx->serverIdColumnName.c_str());
}

void HostResourceQueryOption::setHostGroupIdColumnName(
  const std::string &name) const
{
	m_ctx->hostGroupIdColumnName = name;
}

string HostResourceQueryOption::getHostgroupIdColumnName(
  const std::string &tableAlias) const
{
	if (tableAlias.empty())
		return m_ctx->hostGroupIdColumnName;

	return StringUtils::sprintf("%s.%s",
				    tableAlias.c_str(),
				    m_ctx->hostGroupIdColumnName.c_str());
}

void HostResourceQueryOption::setHostIdColumnName(
  const std::string &name) const
{
	m_ctx->hostIdColumnName = name;
}

string HostResourceQueryOption::getHostIdColumnName(
  const std::string &tableAlias) const
{
	if (tableAlias.empty())
		return m_ctx->hostIdColumnName;

	return StringUtils::sprintf("%s.%s",
				    tableAlias.c_str(),
				    m_ctx->hostIdColumnName.c_str());
}

string HostResourceQueryOption::makeConditionHostGroup(
  const HostGroupSet &hostGroupSet, const string &hostGroupIdColumnName)
{
	string hostGrps;
	HostGroupSetConstIterator it = hostGroupSet.begin();
	size_t commaCnt = hostGroupSet.size() - 1;
	for (; it != hostGroupSet.end(); ++it, commaCnt--) {
		const uint64_t hostGroupId = *it;
		if (hostGroupId == ALL_HOST_GROUPS)
			return "";
		hostGrps += StringUtils::sprintf("%"PRIu64, hostGroupId);
		if (commaCnt)
			hostGrps += ",";
	}
	string cond = StringUtils::sprintf(
	  "%s IN (%s)", hostGroupIdColumnName.c_str(), hostGrps.c_str());
	return cond;
}

string HostResourceQueryOption::makeConditionServer(
  const ServerIdSet &serverIdSet, const std::string &serverIdColumnName)
{
	if (serverIdSet.empty())
		return DBClientHatohol::getAlwaysFalseCondition();

	string condition = StringUtils::sprintf(
	  "%s IN (", serverIdColumnName.c_str());

	ServerIdSetConstIterator serverId = serverIdSet.begin();
	bool first = true;
	for (; serverId != serverIdSet.end(); ++serverId) {
		if (first)
			first = false;
		else
			condition += ",";
		condition += StringUtils::sprintf("%"FMT_SERVER_ID, *serverId);
	}
	condition += ")";
	return condition;
}

string HostResourceQueryOption::makeConditionServer(
  const ServerIdType &serverId, const HostGroupSet &hostGroupSet,
  const string &serverIdColumnName, const string &hostGroupIdColumnName,
  const HostgroupIdType &hostgroupId)
{
	string condition;
	condition = StringUtils::sprintf(
	  "%s=%"FMT_SERVER_ID, serverIdColumnName.c_str(), serverId);

	string conditionHostGroup;
	if (hostgroupId == ALL_HOST_GROUPS) {
		conditionHostGroup =
		  makeConditionHostGroup(hostGroupSet, hostGroupIdColumnName);
	} else {
		conditionHostGroup = StringUtils::sprintf(
		  "%s=%"FMT_HOST_GROUP_ID, hostGroupIdColumnName.c_str(),
		  hostgroupId);
	}
	if (!conditionHostGroup.empty()) {
		return StringUtils::sprintf("(%s AND %s)",
					    condition.c_str(),
					    conditionHostGroup.c_str());
	} else {
		return condition;
	}
}

string HostResourceQueryOption::makeCondition(
  const ServerHostGrpSetMap &srvHostGrpSetMap,
  const string &serverIdColumnName,
  const string &hostGroupIdColumnName,
  const string &hostIdColumnName,
  ServerIdType targetServerId,
  HostgroupIdType targetHostgroupId,
  HostIdType targetHostId)
{
	string condition;

	size_t numServers = srvHostGrpSetMap.size();
	if (numServers == 0) {
		MLPL_DBG("No allowed server\n");
		return DBClientHatohol::getAlwaysFalseCondition();
	}

	if (targetServerId != ALL_SERVERS &&
	    srvHostGrpSetMap.find(targetServerId) == srvHostGrpSetMap.end())
	{
		return DBClientHatohol::getAlwaysFalseCondition();
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
					   hostGroupIdColumnName,
					   targetHostgroupId);
		addCondition(condition, conditionServer, ADD_TYPE_OR);
		++numServers;
	}

	if (targetHostId != ALL_HOSTS) {
		return StringUtils::sprintf("((%s) AND %s=%"FMT_HOST_ID")",
					    condition.c_str(),
					    hostIdColumnName.c_str(),
					    targetHostId);
	}

	if (numServers == 1)
		return condition;
	return StringUtils::sprintf("(%s)", condition.c_str());
}

string HostResourceQueryOption::getFromSectionForOneTable(void)
{
	MLPL_BUG("Not implemented yet: %s\n", __PRETTY_FUNCTION__);
	return "";
}

string HostResourceQueryOption::getFromSectionWithHostgroup(void)
{
	MLPL_BUG("Not implemented yet: %s\n", __PRETTY_FUNCTION__);
	return "";
}

