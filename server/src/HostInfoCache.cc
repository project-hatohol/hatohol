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

#include <cstdio>
#include <map>
#include <string>
#include <ReadWriteLock.h>
#include "Params.h"
#include "HostInfoCache.h"
#include "UnifiedDataStore.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;

typedef map<LocalHostIdType, HostInfoCache::Element> HostIdNameMap;
typedef HostIdNameMap::iterator HostIdNameMapIterator;

struct HostInfoCache::Impl
{
	ReadWriteLock lock;
	HostIdNameMap hostIdNameMap;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HostInfoCache::HostInfoCache(const ServerIdType *serverId)
: m_impl(new Impl())
{
	if (!serverId)
		return;

	HostsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(*serverId);
	ServerHostDefVect svHostDefs;
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
	  uds->getServerHostDefs(svHostDefs, option));
	update(svHostDefs);
}

HostInfoCache::~HostInfoCache()
{
}

void HostInfoCache::update(const ServerHostDef &svHostDef,
                           const HostIdType &hostId, const bool &adhoc)
{
	bool doUpdate = true;
	m_impl->lock.writeLock();
	HostIdNameMapIterator it =
	  m_impl->hostIdNameMap.find(svHostDef.hostIdInServer);
	if (it != m_impl->hostIdNameMap.end()) {
		const Element &elem = it->second;
		const string &hostName = elem.name;
		if (hostName == svHostDef.name)
			doUpdate = false;
	}
	if (doUpdate) {
		Element elem;
		elem.hostId =
		  (hostId != INVALID_HOST_ID) ? hostId : svHostDef.hostId;
		elem.name = svHostDef.name;
		HATOHOL_ASSERT(adhoc || elem.hostId != INVALID_HOST_ID,
		               "INVALID_HOST_ID: server: %d, host: %s\n",
		               svHostDef.serverId,
		               svHostDef.hostIdInServer.c_str());
		m_impl->hostIdNameMap[svHostDef.hostIdInServer] = elem;
	}
	m_impl->lock.unlock();
}

void HostInfoCache::update(const ServerHostDefVect &svHostDefs,
                           const HostHostIdMap *hostHostIdMapPtr)
{
	struct {
		HostIdType operator()(
		  const ServerHostDef &svHostDef,
		  const HostHostIdMap *hostHostIdMapPtr)
		{
			HostIdType hostId = INVALID_HOST_ID;
			if (!hostHostIdMapPtr)
				return hostId;
			HostHostIdMapConstIterator it =
			  hostHostIdMapPtr->find(svHostDef.hostIdInServer);
			if (it != hostHostIdMapPtr->end())
				hostId = it->second;
			return hostId;
		}
	} getHostId;

	// TODO: consider if DBTablesHost should have the cache
	ServerHostDefVectConstIterator svHostDefIt = svHostDefs.begin();
	for (; svHostDefIt != svHostDefs.end(); ++svHostDefIt) {
		const ServerHostDef &svHostDef = *svHostDefIt;
		update(svHostDef, getHostId(svHostDef, hostHostIdMapPtr));
	}
}

bool HostInfoCache::getName(
  const LocalHostIdType &id, Element &elem) const
{
	bool found = false;
	m_impl->lock.readLock();
	HostIdNameMapIterator it = m_impl->hostIdNameMap.find(id);
	if (it != m_impl->hostIdNameMap.end()) {
		elem = it->second;
		found = true;
	}
	m_impl->lock.unlock();
	return found;
}

void HostInfoCache::registerAdHoc(const LocalHostIdType &idInServer,
                                  const string &name, Element &elem)
{
	ServerHostDef svHostDef;
	svHostDef.id             = -1;
	svHostDef.hostId         = INVALID_HOST_ID;
	svHostDef.serverId       = INVALID_SERVER_ID;
	svHostDef.hostIdInServer = idInServer;
	svHostDef.name           = name;
	svHostDef.status         = HOST_STAT_INAPPLICABLE;
	update(svHostDef, INVALID_HOST_ID, true);

	elem.hostId = svHostDef.hostId;
	elem.name   = svHostDef.name;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
