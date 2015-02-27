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

#ifndef DBTablesHost_h
#define DBTablesHost_h

#include "DBTables.h"
#include "DataQueryOption.h"
#include "HostResourceQueryOption.h"

enum HostStatus {
	HOST_STAT_ALL    = -1,
	HOST_STAT_NORMAL = 0,
	HOST_STAT_REMOVED,
	HOST_STAT_SELF_MONITOR,
	HOST_STAT_INAPPLICABLE,
};

struct Host {
	HostIdType  id;
	std::string name;
};

struct ServerHostDef {
	GenericIdType id;
	HostIdType    hostId;
	ServerIdType  serverId;
	std::string   hostIdInServer;
	std::string   name;
	HostStatus    status;
};

typedef std::vector<ServerHostDef> ServerHostDefVect;
typedef ServerHostDefVect::iterator       ServerHostDefVectIterator;
typedef ServerHostDefVect::const_iterator ServerHostDefVectConstIterator;

struct HostAccess {
	GenericIdType id;
	HostIdType    hostId;
	std::string   ipAddrOrFQDN;
	int           priority;
};

struct VMInfo {
	GenericIdType id;
	HostIdType    hostId;
	HostIdType    hypervisorHostId;
};

struct Hostgroup {
	GenericIdType id;
	ServerIdType  serverId;
	std::string   idInServer;
	std::string   name;
};

typedef std::vector<Hostgroup>        HostgroupVect;
typedef HostgroupVect::iterator       HostgroupVectIterator;
typedef HostgroupVect::const_iterator HostgroupVectConstIterator;

struct HostgroupMember {
	GenericIdType id;
	ServerIdType  serverId;
	std::string   hostIdInServer;
	std::string   hostgroupIdInServer;
};

typedef std::vector<HostgroupMember>        HostgroupMemberVect;
typedef HostgroupMemberVect::iterator       HostgroupMemberVectIterator;
typedef HostgroupMemberVect::const_iterator HostgroupMemberVectConstIterator;

enum {
	IDX_HOST_LIST_ID,
	IDX_HOST_LIST_NAME,
	NUM_IDX_HOST_LIST
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

enum {
	IDX_HOST_ACCESS_ID,
	IDX_HOST_ACCESS_HOST_ID,
	IDX_HOST_ACCESS_IP_ADDR_OR_FQDN,
	IDX_HOST_ACCESS_PRIORITY,
	NUM_IDX_HOST_ACCESS
};

enum {
	IDX_HOST_VM_LIST_ID,
	IDX_HOST_VM_LIST_HOST_ID,
	IDX_HOST_VM_LIST_HYPERVISOR_HOST_ID,
	NUM_IDX_VM_LIST
};

enum {
	IDX_HOSTGROUP_LIST_ID,
	IDX_HOSTGROUP_LIST_SERVER_ID,
	IDX_HOSTGROUP_LIST_ID_IN_SERVER,
	IDX_HOSTGROUP_LIST_NAME,
	NUM_IDX_HOSTGROUP_LIST,
};

enum {
	IDX_HOSTGROUP_MEMBER_ID,
	IDX_HOSTGROUP_MEMBER_SERVER_ID,
	IDX_HOSTGROUP_MEMBER_HOST_ID,
	IDX_HOSTGROUP_MEMBER_GROUP_ID,
	NUM_IDX_HOSTGROUP_MEMBER,
};

extern const DBAgent::TableProfile tableProfileHostList;
extern const DBAgent::TableProfile tableProfileServerHostDef;
extern const DBAgent::TableProfile tableProfileHostAccess;
extern const DBAgent::TableProfile tableProfileVMList;
extern const DBAgent::TableProfile tableProfileHostgroupList;
extern const DBAgent::TableProfile tableProfileHostgroupMember;

class HostsQueryOption : public HostResourceQueryOption {
public:
	HostsQueryOption(const UserIdType &userId = INVALID_USER_ID);
	HostsQueryOption(DataQueryContext *dataQueryContext);
	virtual ~HostsQueryOption();

	virtual std::string getCondition(void) const override;

	void setStatus(const HostStatus &status);
	HostStatus getStatus(void) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

// TODO: move the class form DBTablesMonitoring.
class HostgroupsQueryOption;

class HostgroupMembersQueryOption: public HostResourceQueryOption {
public:
	HostgroupMembersQueryOption(const UserIdType &userId = INVALID_USER_ID);
	HostgroupMembersQueryOption(DataQueryContext *dataQueryContext);
};

class DBTablesHost : public DBTables {
public:
	static const int TABLES_VERSION;

	static void reset(void);
	static const SetupInfo &getConstSetupInfo(void);

	DBTablesHost(DBAgent &dbAgent);
	virtual ~DBTablesHost();

	/**
	 * Add a host.
	 *
	 * @param name
	 * A name of the added host. It can be duplicated.
	 *
	 * @return
	 * The unique host ID of the added host.
	 */
	HostIdType addHost(const std::string &name);

	/**
	 * Insert or update a host in both host_list and server_host_def
	 * tables.
	 *
	 * If there's the record whose ID is equal to serverHostDef.id or
	 * with the combination of serverHostDef.serverId and
	 * serverHostDef.hostIdInserver, the record is updated.
	 * If serverHostDef.id is AUTO_INCREMENT_VALUE, a new record is always
	 * added except for the latter case of the previous sentence.
	 * In addition, if serverHostDef.hostId is AUTO_ASSIGNED_ID,
	 * a new record is added in host_list.
	 * serverHostDef.name is used as host_list.name of
	 * the new record.
	 *
	 * @param serverHostDef A data to be inserted/updated.
	 * @param useTransaction A flag to select to use a transaction.
	 * @return
	 * The Host ID of inserted/updated record to/in host_list.
	 */
	HostIdType upsertHost(const ServerHostDef &serverHostDef,
	                      const bool &useTransaction = true);

	void upsertHosts(const ServerHostDefVect &serverHostDefs);

	/**
	 * Insert or update a record to/in the server-host-definition table
	 *
	 * If there's the record whose ID is equal to serverHostDef.id or
	 * with the combination of serverHostDef.serverId and
	 * serverHostDef.hostIdInserver, the record is updated.
	 * If serverHostDef.id is AUTO_INCREMENT_VALUE, a new record is always
	 * added except for the latter case of the previous sentence.
	 *
	 * @param serverHostDef A data to be inserted/updated.
	 * @return
	 * The ID of inserted/updated record.
	 */
	GenericIdType upsertServerHostDef(const ServerHostDef &serverHostDef);

	/**
	 * Insert or update a record to/in the host_access table
	 *
	 * If there's the record whose ID is equal to hostAccess.id,
	 * the record is updated.
	 *
	 * @param hostAccess A data to be inserted/updated.
	 * @return
	 * The ID of inserted/updated record.
	 */
	GenericIdType upsertHostAccess(const HostAccess &hostAccess);

	/**
	 * Insert or update a record to/in the host_access table
	 *
	 * If there's the record whose ID is equal to vmInfo.id,
	 * the record is updated.
	 *
	 * @param vmInfo A data to be inserted/updated.
	 * @return
	 * The ID of inserted/updated record.
	 */
	GenericIdType upsertVMInfo(const VMInfo &vmInfo);

	/**
	 * Insert or update a record to/in the hostgroup_list table
	 *
	 * If there's the record whose ID is equal to hostgroup.id,
	 * the record is updated.
	 *
	 * @param hostgroup A data to be inserted/updated.
	 * @param useTransaction A flag to use a transaction.
	 * @return
	 * The ID of inserted/updated record.
	 */
	GenericIdType upsertHostgroup(const Hostgroup &hostgroup,
	                              const bool &useTransaction = true);

	void upsertHostgroups(const HostgroupVect &hostgroups);

	/**
	 * Get hostgroups.
	 *
	 * @param hostgroups Obtained host groups are stored here.
	 * @param option     An option to query hostgroups.
	 *
	 * @return An HatoholError.
	 */
	HatoholError getHostgroups(HostgroupVect &hostgroups,
	                           const HostgroupsQueryOption &option);

	/**
	 * Insert or update a record to/in the hostgroup_member table
	 *
	 * If there's the record whose ID is equal to hostHostgroup.id,
	 * the record is updated.
	 *
	 * @param hostgroupMember A data to be inserted/updated.
	 * @param useTransaction A flag to use a transaction.
	 * @return
	 * The ID of inserted/updated record.
	 */
	GenericIdType upsertHostgroupMember(
	  const HostgroupMember &hostgroupMember,
	  const bool &useTransaction = true);

	void upsertHostgroupMembers(
	  const HostgroupMemberVect &hostgroupMembers);

	HatoholError getHostgroupMembers(
	  HostgroupMemberVect &hostgrpMembers,
	  const HostgroupMembersQueryOption &option);

	/**
	 * Get the virtual mechines
	 *
	 * @param virtualMachines
	 * An array of the virtual machines is added in it.
	 *
	 * @param hypervisorHostId
	 * A target host ID.
	 *
	 * @param option
	 * A HostsQueryOption instance.
	 *
	 * @param return
	 * If the search is successfully done, HTERR_OK is returned even when
	 * there's no virtual machines.
	 */
	HatoholError getVirtualMachines(HostIdVector &virtualMachines,
	                                const HostIdType &hypervisorHostId,
	                                const HostsQueryOption &option);
	/**
	 * Get the hyperviosr.
	 *
	 * @param hypervisorHostId
	 * The host ID of the found hypervisor is stored in it.
	 *
	 * @param hostId
	 * A target host ID.
	 *
	 * @param option
	 * A HostsQueryOption instance.
	 *
	 * @param return
	 * If hypervisor is successfully found, HTERR_OK is returned.
	 */
	HatoholError getHypervisor(HostIdType &hypervisorHostId,
	                           const HostIdType &hostId,
	                           const HostsQueryOption &option);

	bool isAccessible(
	  const HostIdType &hostId, const HostsQueryOption &option);

	/**
	 * Get hosts.
	 *
	 * @param svHostDevVect The obtained hosts are added to this object.
	 * @param option        A option for the inquiry.
	 *
	 * @return An error status.
	 */
	HatoholError getServerHostDefs(ServerHostDefVect &svHostDefVect,
	                               const HostsQueryOption &option);

protected:
	static SetupInfo &getSetupInfo(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // DBTablesHost_h
