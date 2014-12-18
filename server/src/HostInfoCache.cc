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

using namespace std;
using namespace mlpl;

typedef map<HostIdType, string> HostIdNameMap;
typedef HostIdNameMap::iterator HostIdNameMapIterator;

struct HostInfoCache::Impl
{
	ReadWriteLock lock;
	HostIdNameMap hostIdNameMap;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HostInfoCache::HostInfoCache(void)
: m_impl(new Impl())
{
}

HostInfoCache::~HostInfoCache()
{
}

void HostInfoCache::update(const HostInfo &hostInfo)
{
	bool doUpdate = true;
	m_impl->lock.writeLock();
	HostIdNameMapIterator it = m_impl->hostIdNameMap.find(hostInfo.id);
	if (it != m_impl->hostIdNameMap.end()) {
		const string &hostName = it->second;
		if (hostName == hostInfo.hostName)
			doUpdate = false;
	}
	if (doUpdate)
		m_impl->hostIdNameMap[hostInfo.id] = hostInfo.hostName;;
	m_impl->lock.unlock();
}

bool HostInfoCache::getName(const HostIdType &id, string &name) const
{
	bool found = false;
	m_impl->lock.readLock();
	HostIdNameMapIterator it = m_impl->hostIdNameMap.find(id);
	if (it != m_impl->hostIdNameMap.end()) {
		name = it->second;
		found = true;
	}
	m_impl->lock.unlock();
	return found;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
