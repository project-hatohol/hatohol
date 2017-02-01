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

#pragma once
#include <string>
#include "DBTablesMonitoring.h"

/**
 * Currently This class has only the ID and the name. In addition,
 * Hosts with the same serverId can be cached for an instance.
 */
class HostInfoCache {
public:
	struct Element {
		HostIdType  hostId;
		std::string name;
	};

	HostInfoCache(const ServerIdType *serverId = NULL);
	virtual ~HostInfoCache();
	void update(const ServerHostDef &svHostDef,
	            const HostIdType &hostId = INVALID_HOST_ID,
	            const bool &adhoc = false);
	void update(const ServerHostDefVect &svHostDefs,
	            const HostHostIdMap *hostHostIdMapPtr = NULL);

	/**
	 * Get the name corresponding to the specified host ID.
	 *
	 * @param idInServer A target host ID in the server.
	 * @param elem The obtained element is stored in this paramter.
	 *
	 * @return true if the host is found, or false.
	 */
	bool getName(const LocalHostIdType &idInServer, Element &elem) const;

	/**
	 * Register host information temporarily.
	 *
	 * The host is typically supposed not to be stored in the DB.
	 *
	 * @param idInServer A target host ID in the server.
	 * @param name       A host name
	 * @param elem
	 * The information about the host is stored in this paramter.
	 */
	void registerAdHoc(const LocalHostIdType &idInServer,
	                   const std::string &name, Element &elem);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

