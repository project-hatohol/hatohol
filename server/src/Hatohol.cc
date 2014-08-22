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
	DBTablesConfig::init();
	DBTablesUser::init();
	DBTablesMonitoring::init();
	DBTablesAction::init();

	ItemData::init();

	FaceRest::init();
}

static void reset(const CommandLineOptions *cmdLineOpts)
{
	ChildProcessManager::getInstance()->reset();
	ActorCollector::reset();
	SessionManager::reset();
	ConfigManager::reset(cmdLineOpts);

	DBAgentSQLite3::reset();
	DBHatohol::reset();
	DBClient::reset();
	DBTablesConfig::reset(); // must be after DBClient::reset()
	DBTablesUser::reset();
	DBTablesAction::reset(); // must be after DBTablesConfig::reset()
	DBTablesHost::reset();   // must be after DBHatohol::reset()

	ActionManager::reset();
	ThreadLocalDBCache::reset();

	UnifiedDataStore::getInstance()->reset();
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
