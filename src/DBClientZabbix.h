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

#ifndef DBClientZabbix_h
#define DBClientZabbix_h

#include "DBAgent.h"

class DBClientZabbix {
public:
	static void init(void);
	DBClientZabbix(int zabbixServerId);
	virtual ~DBClientZabbix();

protected:
	static void createTablesIfNeeded(DBDomainId domainId);
	static void dbFileSetupCallback(DBDomainId domainId);
	void createTableTriggersRaw2_0(void);

private:
	DBAgent *m_dbAgent;
};

#endif // DBClientZabbix_h
