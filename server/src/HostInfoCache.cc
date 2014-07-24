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

struct HostInfoCache::PrivateContext
{
	ReadWriteLock lock;
	HostIdNameMap hostIdNameMap;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HostInfoCache::HostInfoCache(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

HostInfoCache::~HostInfoCache()
{
	if (m_ctx)
		delete m_ctx;
}

void HostInfoCache::update(const HostInfo &hostInfo)
{
	bool doUpdate = true;
	m_ctx->lock.writeLock();
	HostIdNameMapIterator it = m_ctx->hostIdNameMap.find(hostInfo.id);
	if (it != m_ctx->hostIdNameMap.end()) {
		const string &hostName = it->second;
		if (hostName == hostInfo.hostName)
			doUpdate = false;
	}
	if (doUpdate)
		m_ctx->hostIdNameMap[hostInfo.id] = hostInfo.hostName;;
	m_ctx->lock.unlock();
}

bool HostInfoCache::getName(const HostIdType &id, string &name) const
{
	bool found = false;
	m_ctx->lock.readLock();
	HostIdNameMapIterator it = m_ctx->hostIdNameMap.find(id);
	if (it != m_ctx->hostIdNameMap.end()) {
		name = it->second;
		found = true;
	}
	m_ctx->lock.unlock();
	return found;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
