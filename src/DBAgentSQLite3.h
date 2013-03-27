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

#ifndef DBAgentSQLite3_h
#define DBAgentSQLite3_h

#include <sqlite3.h>

#include "DBAgent.h"

class DBAgentSQLite3 {
public:
	static void init(void);
	DBAgentSQLite3(void);
	virtual ~DBAgentSQLite3();

	// virtual methods
	virtual void
	   addTargetServer(MonitoringServerInfo *monitoringServerInfo);
	virtual void
	   getTargetServers(MonitoringServerInfoList &monitoringServers);

protected:
	void openDatabase(void);

private:
	sqlite3 *m_db;
	string   m_dbPath;
};

#endif // DBAgentSQLite3_h
