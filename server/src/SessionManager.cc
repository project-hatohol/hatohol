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
#include "SessionManager.h"
#include "MutexLock.h"
using namespace mlpl;

struct SessionManager::PrivateContext {
	static MutexLock initLock;
	static SessionManager *instance;
};

SessionManager *SessionManager::PrivateContext::instance = NULL;
MutexLock SessionManager::PrivateContext::initLock;


// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SessionManager *SessionManager::getInstance(void)
{
	PrivateContext::initLock.lock();
	if (!PrivateContext::instance)
		PrivateContext::instance = new SessionManager();
	PrivateContext::initLock.unlock();
	return PrivateContext::instance;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SessionManager::SessionManager(void)
: m_ctx(NULL)
{
}

SessionManager::~SessionManager()
{
}
