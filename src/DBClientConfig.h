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

#ifndef DBClientConfig_h
#define DBClientConfig_h

#include <list>
#include "DBClient.h"

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

class DBClientConfig : public DBClient {
public:
	static void reset(void);

	DBClientConfig(void);
	virtual ~DBClientConfig();

	void addTargetServer(MonitoringServerInfo *monitoringServerInfo);
	void getTargetServers(MonitoringServerInfoList &monitoringServers);

protected:
	static void resetDBInitializedFlags(void);
	void prepareSetupFunction(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientConfig_h
