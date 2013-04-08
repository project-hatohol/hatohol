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

#ifndef DBAgent_h
#define DBAgent_h

#include <string>
#include <list>
using namespace std;

#include <glib.h>
#include <stdint.h>

#include "SQLProcessorTypes.h"

enum MonitoringSystemType {
	MONITORING_SYSTEM_ZABBIX,
};

struct MonitoringServerInfo {
	uint32_t             id;
	MonitoringSystemType type;
	string               hostName;
	string               ipAddress;
	string               nickname;
};

typedef list<MonitoringServerInfo>         MonitoringServerInfoList;
typedef MonitoringServerInfoList::iterator MonitoringServerInfoListIterator;

enum TriggerStatusType {
	TRIGGER_STATUS_OK,
	TRIGGER_STATUS_PROBLEM,
};

enum TriggerSeverityType {
	TRIGGER_SEVERITY_INFO,
	TRIGGER_SEVERITY_WARN,
};

struct TriggerInfo {
	uint64_t            id;
	TriggerStatusType   status;
	TriggerSeverityType severity;
	timespec            lastChangeTime;
	uint32_t            serverId;
	string              hostId;
	string              hostName;
	string              brief;
};

typedef list<TriggerInfo>         TriggerInfoList;
typedef TriggerInfoList::iterator TriggerInfoListIterator;

struct DBAgentTableCreationArg {
	string              tableName;
	size_t              numColumns;
	const ColumnDef    *columnDefs;
};

union DBAgentValue {
	int         vInt;
	uint64_t    vUint64;
	const char *vString;
	double      vDouble;
};

struct DBAgentInsertArg {
	string tableName;
	vector<DBAgentValue> row;
	size_t              numColumns;
	const ColumnDef    *columnDefs;
};

struct DBAgentSelectArg {
	string tableName;
	const ColumnDef    *columnDefs;
	vector<size_t>      columnIndexes;

	// output
	ItemDataVector      dataVector;
};

typedef uint32_t DBDomainId;
typedef void (*DBSetupFunc)(DBDomainId domainId);
static const DBDomainId DefaultDBDomainId = 0;

class DBAgent {
public:
	static void
	   addSetupFunction(DBDomainId domainId, DBSetupFunc setupFunc);

	DBAgent(DBDomainId = DefaultDBDomainId);
	virtual ~DBAgent();

	// virtual methods
	virtual bool isTableExisting(const string &tableName) = 0;
	virtual bool isRecordExisting(const string &tableName,
	                              const string &condition) = 0;
	virtual void
	   addTargetServer(MonitoringServerInfo *monitoringServerInfo) = 0;
	virtual void
	   getTargetServers(MonitoringServerInfoList &monitoringServers) = 0;

	virtual void createTable(DBAgentTableCreationArg &tableCreationArg) = 0;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBAgent_h
