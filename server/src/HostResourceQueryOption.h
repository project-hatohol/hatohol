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

#pragma once
#include <string>
#include "Params.h"
#include "DBAgent.h"
#include "DataQueryOption.h"

class HostResourceQueryOption : public DataQueryOption {
public:
	struct Synapse {
		const DBAgent::TableProfile &tableProfile;
		const size_t                 selfIdColumnIdx;
		const size_t                 serverIdColumnIdx;

		const DBAgent::TableProfile &hostTableProfile;
		const size_t                 hostIdColumnIdx;
		const bool                   needToJoinHostgroup;

		const DBAgent::TableProfile &hostgroupMapTableProfile;
		const size_t                 hostgroupMapServerIdColumnIdx;
		const size_t                 hostgroupMapHostIdColumnIdx;
		const size_t                 hostgroupMapGroupIdColumnIdx;

		const size_t                 globalHostIdColumnIdx;
		const size_t                 hostgroupMapGlobalHostIdColumnIdx;
		
		Synapse(const DBAgent::TableProfile &tableProfile,
		     const size_t &selfIdColumnIdx,
		     const size_t &serverIdColumnIdx,
		     const DBAgent::TableProfile &hostTableProfile,
		     const size_t &hostIdColumnIdx,
		     const bool &needToJoinHostgroup,
		     const DBAgent::TableProfile &hostgroupMapTableProfile,
		     const size_t &hostgroupMapServerIdColumnIdx,
		     const size_t &hostgroupMapHostIdColumnIdx,
		     const size_t &hostgroupMapGroupIdColumnIdx,
		     const size_t &globalHostIdColumnIdx
		       = INVALID_COLUMN_IDX,
		     const size_t &hostgroupMapGlobalHostIdColumnIdx
		       = INVALID_COLUMN_IDX);
	};

	HostResourceQueryOption(const Synapse &synapse,
	                        const UserIdType &userId = INVALID_USER_ID);
	HostResourceQueryOption(const Synapse &synapse,
	                        DataQueryContext *dataQueryContext);
	HostResourceQueryOption(const HostResourceQueryOption &src);
	virtual ~HostResourceQueryOption();

	/**
	 * Get the primary table name.
	 *
	 * @return The primary table name.
	 */
	const char *getPrimaryTableName(void) const;

	virtual std::string getCondition(void) const override;

	/**
	 * Get a part of an SQL statement for a FROM clause.
	 *
	 * @return A string for a FROM clause.
	 */
	virtual std::string getFromClause(void) const;

	/**
	 * Get information if the host group should be used.
	 *
	 * If synapse.tableProfile in the contructor is identical to
	 * synapse.hostgroupMapTableProfile, this method returns false even
	 * if the target host group is being specified.
	 *
	 * @return true if the host group should used. Otherwiser, false.
	 */
	virtual bool isHostgroupUsed(void) const;

	/**
	 * Get a column name at the specified index.
	 * If a target host group is specified or useTableNameAlways(),
	 * the returned form has the table name suc as tableName.colunName.
	 *
	 * @return A column name.
	 */
	virtual std::string getColumnName(const size_t &idx) const;

	/**
	 * Get a hostgroup column name of the specified index.
	 * If a target host group is specified or useTableNameAlways(),
	 * the returned form has the table name suc as tableName.colunName.
	 *
	 * @return A column name.
	 */
	virtual std::string getHostgroupColumnName(const size_t &idx) const;


	virtual ServerIdType getTargetServerId(void) const;
	virtual void setTargetServerId(const ServerIdType &targetServerId);
	virtual LocalHostIdType getTargetHostId(void) const;
	virtual void setTargetHostId(const LocalHostIdType &targetHostId);
	virtual HostgroupIdType getTargetHostgroupId(void) const;
	virtual void setTargetHostgroupId(HostgroupIdType targetHostgroupId);

	/**
	 * Set a list of IDs of monitoring servers to select.
	 *
	 * @param serverIds
	 * A list of IDs of monitoring servers to select.
	 */
	virtual void setSelectedServerIds(const ServerIdSet &serverIds);
	virtual const ServerIdSet &getSelectedServerIds(void);

	/**
	 * Set a list of IDs of monitoring servers to exclude.
	 *
	 * @param serverIds
	 * A list of IDs of monitoring servers to exclude.
	 */
	virtual void setExcludedServerIds(const ServerIdSet &serverIds);
	virtual const ServerIdSet &getExcludedServerIds(void);

	/**
	 * Set a list of IDs of hostgroups to select.
	 *
	 * @param hostgroupIds
	 * A list of IDs of hostgroups to select.
	 */
	virtual void setSelectedHostgroupIds(
	  const ServerHostGrpSetMap &hostgroupIds);
	virtual const ServerHostGrpSetMap &getSelectedHostgroupIds(void);

	/**
	 * Set a list of IDs of hostgroups to exclude.
	 *
	 * @param hostgroupIds
	 * A list of IDs of hostgroups to exclude.
	 */
	virtual void setExcludedHostgroupIds(
	  const ServerHostGrpSetMap &hostgroupIds);
	virtual const ServerHostGrpSetMap &getExcludedHostgroupIds(void);

	/**
	 * Set a list of IDs of hosts to select.
	 *
	 * @param hostIds
	 * A list of IDs of hosts to select.
	 */
	virtual void setSelectedHostIds(const ServerHostSetMap &hostIds);
	virtual const ServerHostSetMap &getSelectedHostIds(void);

	/**
	 * Set a list of IDs of hosts to exclude.
	 *
	 * @param hostIds
	 * A list of IDs of hosts to exclude.
	 */
	virtual void setExcludedHostIds(const ServerHostSetMap &hostIds);
	virtual const ServerHostSetMap &getExcludedHostIds(void);

	/**
	 * Enable or disable the filter to exclude defunct servers.
	 *
	 * @param enable
	 * If the parameter is true, the filter is enabled. Otherwise,
	 * it is disabled.
	 *
	 */
	void setExcludeDefunctServers(const bool &enable = true);

	/**
	 * Get the status of the filter to exclude defunct servers.
	 *
	 * @return
	 * If the filter is enabled, true is returned, Otherwise,
	 * false is returned.
	 *
	 */
	const bool &getExcludeDefunctServers(void) const;

	std::string getJoinClause(void) const;

protected:
	std::string getServerIdColumnName(void) const;
	std::string getHostgroupIdColumnName(void) const;
	std::string getHostIdColumnName(void) const;
	bool isAllowedServer(const ServerIdType &targetServerId) const;
	bool isAllowedHostgroup(const ServerIdType &targetServerId,
				const HostgroupIdType &targetHostgroupId) const;

	std::string makeConditionTargetIds(void) const;
	std::string makeConditionAllowedHosts(void) const;
	std::string makeConditionServer(
	  const ServerIdSet &serverIdSet,
	  const std::string &serverIdColumnName) const;
	std::string makeConditionServer(
	  const ServerIdType &serverId,
	  const HostgroupIdSet &hostgroupIdSet,
	  const std::string &serverIdColumnName,
	  const std::string &hostgroupIdColumnName,
	  const HostgroupIdType &hostgroupId = ALL_HOST_GROUPS) const;
	std::string makeConditionHostgroup(
	  const HostgroupIdSet &hostgroupIdSet,
	  const std::string &hostgroupIdColumnName) const;

	std::string makeConditionSelectedServers(void) const;
	std::string makeConditionExcludedServers(void) const;
	std::string makeConditionSelectedHostgroups(void) const;
	std::string makeConditionExcludedHostgroups(void) const;
	std::string makeConditionSelectedHosts(void) const;
	std::string makeConditionExcludedHosts(void) const;
	std::string makeConditionHostsFilter(void) const;

	virtual std::string getFromClauseForOneTable(void) const;
	virtual std::string getFromClauseWithHostgroup(void) const;

	std::string getColumnNameCommon(
	  const DBAgent::TableProfile &tableProfile, const size_t &idx) const;
	bool isHostgroupEnumerationInCondition(void) const;
	std::string getJoinClauseWithGlobalHostId(void) const;

	const ServerIdSet &getValidServerIdSet(void) const;
	const ServerHostGrpSetMap &getAllowedServersAndHostgroups(void) const;

	// For test use only
	void setValidServerIdSet(const ServerIdSet *set);
	void setAllowedServersAndHostgroups(const ServerHostGrpSetMap *map);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

