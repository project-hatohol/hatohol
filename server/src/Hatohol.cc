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

#include <MutexLock.h>
using namespace mlpl;

#include "Hatohol.h"
#include "Utils.h"
#include "SQLUtils.h"
#include "SQLProcessorZabbix.h"
#include "SQLProcessorFactory.h"
#include "SQLProcessorSelect.h"
#include "SQLProcessorInsert.h"
#include "SQLProcessorUpdate.h"
#include "FaceMySQLWorker.h"
#include "FaceRest.h"
#include "HatoholException.h"
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"
#include "DBClientConfig.h"
#include "DBClientHatohol.h"
#include "DBClientZabbix.h"
#include "ActionManager.h"
#include "ActorCollector.h"
#include "DBClientAction.h"

static MutexLock mutex;
static bool initDone = false; 

static void init(const CommandLineArg *arg)
{
	Utils::init();
	HatoholException::init();

	DBAgentSQLite3::init();
	DBAgentMySQL::init();
	DBClientConfig::init(arg);
	DBClientHatohol::init();
	DBClientZabbix::init();
	DBClientAction::init();

	ItemData::init();
	SQLUtils::init();
	SQLFormulaParser::init();
	SQLColumnParser::init(); // must be put after SQLFormulaParser::init()
	SQLWhereParser::init();  // must be put after SQLFormulaParser::init()
	SQLFromParser::init();
	FaceMySQLWorker::init();
	SQLProcessorSelect::init();
	SQLProcessorInsert::init();
	SQLProcessorUpdate::init();
	SQLProcessorZabbix::init();
	SQLProcessorFactory::init();

	ActorCollector::init();
	FaceRest::init();
}

static void reset(void)
{
	DBAgentSQLite3::reset();
	DBClientConfig::reset();
	DBClientAction::reset();
	DBClientHatohol::reset();
	DBClientZabbix::reset();

	ActionManager::reset();
	ActorCollector::reset();
}

void hatoholInit(const CommandLineArg *arg)
{
	mutex.lock();
	if (!initDone) {
		init(arg);
		initDone = true;
	}
	reset();
	mutex.unlock();
}
