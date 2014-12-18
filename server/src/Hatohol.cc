/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <glib.h>

#include <Mutex.h>
using namespace mlpl;

#include "Hatohol.h"
#include "Utils.h"
#include "ConfigManager.h"
#include "FaceRest.h"
#include "HatoholException.h"
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"
#include "DBTablesConfig.h"
#include "DBTablesMonitoring.h"
#include "ActionManager.h"
#include "ActorCollector.h"
#include "DBTablesUser.h"
#include "DBTablesAction.h"
#include "ThreadLocalDBCache.h"
#include "SessionManager.h"
#include "UnifiedDataStore.h"
#include "ChildProcessManager.h"
#include "DBTablesHost.h"

static Mutex mutex;
static bool initDone = false; 

static void init(void)
{
	Utils::init();
	HatoholError::init();
	HatoholException::init();

	DBAgentSQLite3::init();
	DBAgentMySQL::init();
	DBTablesUser::init();
	DBTablesAction::init();

	ItemData::init();

	FaceRest::init();
}

static void reset(const CommandLineOptions *cmdLineOpts)
{
	ChildProcessManager::getInstance()->reset();
	ActorCollector::reset();
	SessionManager::reset();

	DBHatohol::reset();

	// These should be place after DBHatohol::reset()
	DBTablesConfig::reset();
	DBTablesUser::reset();
	DBTablesAction::reset();
	DBTablesHost::reset();
	DBTablesMonitoring::reset();

	ActionManager::reset();
	ThreadLocalDBCache::reset();

	UnifiedDataStore::getInstance()->reset();

	ConfigManager::reset(cmdLineOpts);
}

void hatoholInit(const CommandLineOptions *cmdLineOpts)
{
	mutex.lock();
	if (!initDone) {
		init();
		initDone = true;
	}
	reset(cmdLineOpts);
	mutex.unlock();
}
