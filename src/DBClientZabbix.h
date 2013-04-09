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
	static const int DB_VERSION;

	DBClientZabbix(size_t zabbixServerId);
	virtual ~DBClientZabbix();

	void addTriggersRaw2_0(ItemTablePtr tablePtr);

protected:
	static void dbSetupFunc(DBDomainId domainId);
	static void createTableSystem(const string &dbPath);
	static void createTableReplicaGeneration(const string &dbPath);
	static void createTableTriggersRaw2_0(const string &dbPath);
	static void updateDBIfNeeded(const string &dbPath);
	static bool getDBVersion(const string &dbPath);
	void prepareSetupFuncCallback(size_t zabbixServerId);
	int getLatestTriggersGenerationId(void);
	int updateReplicaGeneration(void);
	void addTriggersRaw2_0WithTryBlock(int generationId,
	                                   ItemTablePtr tablePtr);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientZabbix_h
