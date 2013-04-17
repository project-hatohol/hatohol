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
	enum {
		REPLICA_GENERATION_TARGET_ID_TRIGGER,
		REPLICA_GENERATION_TARGET_ID_FUNCTION,
		NUM_REPLICA_GENERATION_TARGET_ID,
	};

	static const int DB_VERSION;
	static const int NUM_PRESERVED_GENRATIONS_TRIGGERS;
	static const int REPLICA_GENERATION_NONE;

	static string getDBPath(size_t zabbixServerId);
	static void resetDBInitializedFlags(void);

	DBClientZabbix(size_t zabbixServerId);
	virtual ~DBClientZabbix();

	void addTriggersRaw2_0(ItemTablePtr tablePtr);
	void addFunctionsRaw2_0(ItemTablePtr tablePtr);

protected:
	static void dbSetupFunc(DBDomainId domainId);
	static void createTable(const string &dbPath);
	static void createTableSystem(const string &dbPath);
	static void createTableReplicaGeneration(const string &dbPath);
	static void createTableTriggersRaw2_0(const string &dbPath);
	static void createTableFunctionsRaw2_0(const string &dbPath);
	static void updateDBIfNeeded(const string &dbPath);
	static int getDBVersion(const string &dbPath);
	void prepareSetupFuncCallback(size_t zabbixServerId);
	int getLatestTriggersGenerationId(void);
	int updateReplicaGeneration(int replicaTargetId);
	void addReplicatedItems
	  (int generationId, ItemTablePtr tablePtr,
	   const string &tableName, size_t numColumns,
	   const ColumnDef *columnDefs);
	void deleteOldReplicatedItems
	  (int replicaTargetId, const string &tableName,
	   const ColumnDef *columnDefs, int generationIdIdx);
	int getStartIdToRemove(int replicaTargetId);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientZabbix_h
