/*
 * Copyright (C) 2013 Project Hatohol
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
#include "SQLUtils.h"
#include "FaceRest.h"
#include "HatoholException.h"
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"
#include "DBClientConfig.h"
#include "DBClientHatohol.h"
#include "ActionManager.h"
#include "ActorCollector.h"
#include "DBClientAction.h"
#include "DBClientUser.h"
#include "CacheServiceDBClient.h"
#include "SessionManager.h"
#include "UnifiedDataStore.h"
#include "ChildProcessManager.h"
#include "DBCGroupRegular.h"
#include "DBClientHost.h"

static Mutex mutex;
static bool initDone = false; 

static void init(const CommandLineArg &arg)
{
	Utils::init();
	HatoholError::init();
	HatoholException::init();

	DBAgentSQLite3::init();
	DBAgentMySQL::init();
	DBClientConfig::init();
	DBClientUser::init();
	DBClientHatohol::init();
	DBClientAction::init();
	DBClientHost::init();

	ItemData::init();
	SQLUtils::init();

	FaceRest::init();
}

static void reset(const CommandLineArg &arg)
{
	ChildProcessManager::getInstance()->reset();
	ActorCollector::reset();
	SessionManager::reset();
	ConfigManager::reset();

	DBAgentSQLite3::reset();
	DBClient::reset();
	DBCGroupRegular::reset();
	DBClientConfig::reset(); // must be after DBClient::reset()
	DBClientUser::reset();
	DBClientAction::reset(); // must be after DBClientConfig::reset()

	ActionManager::reset();
	CacheServiceDBClient::reset();

	UnifiedDataStore::getInstance()->reset();
}

void hatoholInit(const CommandLineArg *arg)
{
	CommandLineArg emptyArg;
	if (!arg)
		arg = &emptyArg;
	mutex.lock();
	if (!initDone) {
		init(*arg);
		initDone = true;
	}
	reset(*arg);
	mutex.unlock();
}
