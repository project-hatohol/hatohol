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

#ifndef HostResourceQueryOption_h
#define HostResourceQueryOption_h

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
		
		Synapse(const DBAgent::TableProfile &tableProfile,
		     const size_t &selfIdColumnIdx,
		     const size_t &serverIdColumnIdx,
		     const DBAgent::TableProfile &hostTableProfile,
		     const size_t &hostIdColumnIdx,
		     const bool &needToJoinHostgroup,
		     const DBAgent::TableProfile &hostgroupMapTableProfile,
		     const size_t &hostgroupMapServerIdColumnIdx,
		     const size_t &hostgroupMapHostIdColumnIdx,
		     const size_t &hostgroupMapGroupIdColumnIdx);
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
	virtual HostIdType getTargetHostId(void) const;
	virtual void setTargetHostId(HostIdType targetHostId);
	virtual HostgroupIdType getTargetHostgroupId(void) const;
	virtual void setTargetHostgroupId(HostgroupIdType targetHostgroupId);

	/**
	 * Enable or disable the filter for the data of defunct servers.
	 *
	 * @param enable
	 * If the parameter is true, the filter is enabled. Otherwise,
	 * it is disabled.
	 *
	 */
	void setFilterForDataOfDefunctServers(const bool &enable = true);

	/**
	 * Get the filter status for the data of defunct servers.
	 *
	 * @return
	 * If the filter is enabled, true is returned, Otherwise,
	 * false is returned.
	 *
	 */
	const bool &getFilterForDataOfDefunctServers(void) const;

	virtual const DBTermCodec *getDBTermCodec(void) const override;

	std::string getJoinClause(void) const;

protected:
	std::string getServerIdColumnName(void) const;
	std::string getHostgroupIdColumnName(void) const;
	std::string getHostIdColumnName(void) const;

	static std::string makeCondition(
	  const ServerHostGrpSetMap &srvHostGrpSetMap,
	  const std::string &serverIdColumnName,
	  const std::string &hostgroupIdColumnName,
	  const std::string &hostIdColumnName,
	  ServerIdType targetServerId = ALL_SERVERS,
	  HostgroupIdType targetHostgroup = ALL_HOST_GROUPS,
	  HostIdType targetHostId = ALL_HOSTS);
	static std::string makeConditionServer(
	  const ServerIdSet &serverIdSet,
	  const std::string &serverIdColumnName);
	static std::string makeConditionServer(
	  const ServerIdType &serverId,
	  const HostgroupIdSet &hostgroupIdSet,
	  const std::string &serverIdColumnName,
	  const std::string &hostgroupIdColumnName,
	  const HostgroupIdType &hostgroupId = ALL_HOST_GROUPS);
	static std::string makeConditionHostgroup(
	  const HostgroupIdSet &hostgroupIdSet,
	  const std::string &hostgroupIdColumnName);

	virtual std::string getFromClauseForOneTable(void) const;
	virtual std::string getFromClauseWithHostgroup(void) const;

	std::string getColumnNameCommon(
	  const DBAgent::TableProfile &tableProfile, const size_t &idx) const;
	bool isHostgroupEnumerationInCondition(void) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // HostResourceQueryOption_h
