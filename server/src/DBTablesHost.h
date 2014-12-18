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

struct ServerHostDef {
	GenericIdType id;
	HostIdType    hostId;
	ServerIdType  serverId;
	std::string   hostIdInServer;
	std::string   name;
};

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

struct HostHostgroup {
	GenericIdType id;
	ServerIdType  serverId;
	std::string   hostIdInServer;
	std::string   hostgroupIdInServer;
};

struct HostQueryOption : public DataQueryOption {
public:
	HostQueryOption(const UserIdType &userId = INVALID_USER_ID);
};

class DBTablesHost : public DBTables {
public:
	static const int TABLES_VERSION;

	static void reset(void);

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
	 * Insert or update a record to/in the host_hostgroup table
	 *
	 * If there's the record whose ID is equal to hostHostgroup.id,
	 * the record is updated.
	 *
	 * @param hostHostgroup A data to be inserted/updated.
	 * @return
	 * The ID of inserted/updated record.
	 */
	GenericIdType upsertHostHostgroup(const HostHostgroup &hostHostgroup);

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
	 * A HostQueryOption instance.
	 *
	 * @param return
	 * If the search is successfully done, HTERR_OK is returned even when
	 * there's no virtual machines.
	 */
	HatoholError getVirtualMachines(HostIdVector &virtualMachines,
	                                const HostIdType &hypervisorHostId,
	                                const HostQueryOption &option);
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
	 * A HostQueryOption instance.
	 *
	 * @param return
	 * If hypervisor is successfully found, HTERR_OK is returned.
	 */
	HatoholError getHypervisor(HostIdType &hypervisorHostId,
	                           const HostIdType &hostId,
	                           const HostQueryOption &option);

	bool isAccessible(
	  const HostIdType &hostId, const HostQueryOption &option);

protected:
	static SetupInfo &getSetupInfo(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // DBTablesHost_h
