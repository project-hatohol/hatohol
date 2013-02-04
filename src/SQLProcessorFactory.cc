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

#include "SQLProcessorFactory.h"
#include "SQLProcessorZabbix.h"

GRWLock SQLProcessorFactory::m_lock;
map<string, SQLProcessorCreatorFunc> SQLProcessorFactory::m_factoryMap;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessorFactory::init(void)
{
	addFactory(SQLProcessorZabbix::getDBNameStatic(),
	           SQLProcessorZabbix::createInstance);
}

SQLProcessor *SQLProcessorFactory::create(string &DBName)
{
	map<string, SQLProcessorCreatorFunc>::iterator it;
	SQLProcessor *composer = NULL;

	g_rw_lock_reader_lock(&m_lock);
	it = m_factoryMap.find(DBName);
	if (it != m_factoryMap.end()) {
		SQLProcessorCreatorFunc creator = it->second;
		composer = (*creator)();
	}
	g_rw_lock_reader_unlock(&m_lock);

	return composer;
}

void SQLProcessorFactory::addFactory(string name, SQLProcessorCreatorFunc factory)
{
	g_rw_lock_writer_lock(&m_lock);
	m_factoryMap[name] = factory;
	g_rw_lock_writer_unlock(&m_lock);
}
