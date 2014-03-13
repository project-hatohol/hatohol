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
	struct Bind {
		const DBAgent::TableProfile &tableProfile;
		const size_t                 selfIdColumnIdx;
		const size_t                 serverIdColumnIdx;
		const size_t                 hostIdColumnIdx;

		const DBAgent::TableProfile &hostgroupMapTableProfile;
		const size_t                 hostgroupMapServerIdColumnIdx;
		const size_t                 hostgroupMapHostIdColumnIdx;
		const size_t                 hostgroupMapGroupIdColumnIdx;
		
		Bind(const DBAgent::TableProfile &tableProfile,
		     const size_t &selfIdColumnIdx,
		     const size_t &serverIdColumnIdx,
		     const size_t &hostIdColumnIdx,
		     const DBAgent::TableProfile &hostgroupMapTableProfile,
		     const size_t &hostgroupMapServerIdColumnIdx,
		     const size_t &hostgroupMapHostIdColumnIdx,
		     const size_t &hostgroupMapGroupIdColumnIdx);
	};

	HostResourceQueryOption(const Bind &bind,
	                        const UserIdType &userId = INVALID_USER_ID);
	HostResourceQueryOption(const Bind &bind,
	                        DataQueryContext *dataQueryContext);
	HostResourceQueryOption(const HostResourceQueryOption &src);
	virtual ~HostResourceQueryOption();

	/**
	 * Get the primary table name.
	 *
	 * @return The primary table name.
	 */
	const char *getPrimaryTableName(void) const;

	virtual std::string getCondition(
	  const std::string &tableAlias = "") const; // override

	/**
	 * Generate a part of an SQL statement for a FROM section.
	 */
	virtual std::string getFromSection(void);

	virtual ServerIdType getTargetServerId(void) const;
	virtual void setTargetServerId(const ServerIdType &targetServerId);
	virtual HostIdType getTargetHostId(void) const;
	virtual void setTargetHostId(HostIdType targetHostId);
	virtual HostgroupIdType getTargetHostgroupId(void) const;
	virtual void setTargetHostgroupId(HostgroupIdType targetHostGroupId);

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

protected:
	void setServerIdColumnName(const std::string &name) const;
	std::string getServerIdColumnName(
	  const std::string &tableAlias = "") const;
	void setHostGroupIdColumnName(const std::string &name) const;
	std::string getHostgroupIdColumnName(
	  const std::string &tableAlias = "") const;
	void setHostIdColumnName(const std::string &name) const;
	std::string getHostIdColumnName(
	  const std::string &tableAlias = "") const;
	static std::string makeCondition(
	  const ServerHostGrpSetMap &srvHostGrpSetMap,
	  const std::string &serverIdColumnName,
	  const std::string &hostGroupIdColumnName,
	  const std::string &hostIdColumnName,
	  ServerIdType targetServerId = ALL_SERVERS,
	  HostgroupIdType targetHostgroup = ALL_HOST_GROUPS,
	  HostIdType targetHostId = ALL_HOSTS);
	static std::string makeConditionServer(
	  const ServerIdSet &serverIdSet,
	  const std::string &serverIdColumnName);
	static std::string makeConditionServer(
	  const ServerIdType &serverId,
	  const HostGroupSet &hostGroupSet,
	  const std::string &serverIdColumnName,
	  const std::string &hostGroupIdColumnName,
	  const HostgroupIdType &hostgroupId = ALL_HOST_GROUPS);
	static std::string makeConditionHostGroup(
	  const HostGroupSet &hostGroupSet,
	  const std::string &hostGroupIdColumnName);

	virtual std::string getFromSectionForOneTable(void);
	virtual std::string getFromSectionWithHostgroup(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // HostResourceQueryOption_h
