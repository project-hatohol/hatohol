/*
 * Copyright (C) 2014 Project Hatohol
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

#ifndef DBTablesHost_h
#define DBTablesHost_h

#include "DBTables.h"

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

protected:
	static SetupInfo &getSetupInfo(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // DBTablesHost_h
