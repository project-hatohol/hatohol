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

#include "Asura.h"
#include "glib.h"

#include "SQLUtils.h"
#include "SQLProcessorZabbix.h"
#include "SQLProcessorFactory.h"
#include "SQLProcessorSelect.h"
#include "SQLProcessorInsert.h"
#include "SQLProcessorUpdate.h"
#include "FaceMySQLWorker.h"
#include "AsuraException.h"
#include "DBAgentSQLite3.h"
#include "DBClientZabbix.h"

static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
static bool initDone = false; 

void asuraInit(void)
{
	g_static_mutex_lock(&mutex);
	if (initDone) {
		g_static_mutex_unlock(&mutex);
		return;
	}

	AsuraException::init();

	DBAgentSQLite3::init();
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

	initDone = true;
	g_static_mutex_unlock(&mutex);
}
