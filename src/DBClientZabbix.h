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
		REPLICA_GENERATION_TARGET_ID_ITEM,
		REPLICA_GENERATION_TARGET_ID_HOST,
		NUM_REPLICA_GENERATION_TARGET_ID,
	};
	static const int REPLICA_TARGET_ID_SYSTEM_LATEST_COLUMNS_MAP[];

	static const int DB_VERSION;
	static const int NUM_PRESERVED_GENRATIONS_TRIGGERS;
	static const int REPLICA_GENERATION_NONE;

	static void init(void);
	static string getDBPath(size_t zabbixServerId);
	static void resetDBInitializedFlags(void);

	DBClientZabbix(size_t zabbixServerId);
	virtual ~DBClientZabbix();

	void addTriggersRaw2_0(ItemTablePtr tablePtr);
	void addFunctionsRaw2_0(ItemTablePtr tablePtr);
	void addItemsRaw2_0(ItemTablePtr tablePtr);
	void addHostsRaw2_0(ItemTablePtr tablePtr);

protected:
	typedef void (*CreateTableInitializer)(DBAgent *, void *);

	static void dbSetupFunc(DBDomainId domainId);
	static void createTable
	  (DBAgent *dbAgent, const string &tableName, size_t numColumns,
	   const ColumnDef *columnDefs,
	   CreateTableInitializer initializer = NULL, void *data = NULL);
	static void createTable
	  (const string &dbPath, const string &tableName, size_t numColumns,
	   const ColumnDef *columnDefs,
	   CreateTableInitializer initializer = NULL, void *data = NULL);
	static void tableInitializerSystem(DBAgent *dbAgent, void *data);
	static void updateDBIfNeeded(DBAgent *dbAgent);
	static int getDBVersion(DBAgent *dbAgent);

	void prepareSetupFuncCallback(size_t zabbixServerId);
	int getLatestGenerationId(void);
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
