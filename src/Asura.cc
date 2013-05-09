/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <glib.h>

#include <MutexLock.h>
using namespace mlpl;

#include "Asura.h"
#include "SQLUtils.h"
#include "SQLProcessorZabbix.h"
#include "SQLProcessorFactory.h"
#include "SQLProcessorSelect.h"
#include "SQLProcessorInsert.h"
#include "SQLProcessorUpdate.h"
#include "FaceMySQLWorker.h"
#include "FaceRest.h"
#include "AsuraException.h"
#include "DBAgentSQLite3.h"
#include "DBClientConfig.h"
#include "DBClientAsura.h"
#include "DBClientZabbix.h"

static MutexLock mutex;
static bool initDone = false; 

static void init(void)
{
	AsuraException::init();

	DBAgentSQLite3::init();
	DBClientAsura::init();
	DBClientZabbix::init();

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

	FaceRest::init();
}

static void reset(void)
{
	DBAgentSQLite3::reset();
	DBClientConfig::reset();
	DBClientAsura::reset();
	DBClientZabbix::reset();
}

void asuraInit(void)
{
	mutex.lock();
	if (!initDone) {
		init();
		initDone = true;
	}
	reset();
	mutex.unlock();
}
