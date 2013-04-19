#include <cstdio>
#include <cstdlib>
#include "Asura.h"
#include "DBClientTest.h"
#include "DBAgentSQLite3.h"
#include "DBClientAsura.h"
#include "ConfigManager.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Need one argument for the DB name.");
		return EXIT_FAILURE;
	}

	string dbName(argv[1]);
	printf("DBName: %s\n", dbName.c_str());

	asuraInit();
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_OFFSET_CONFIG, dbName);
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_OFFSET_ASURA, dbName);

	for (size_t i = 0; i < NumServerInfo; i++) {
		MonitoringServerInfo *svInfo = &serverInfo[i];
		DBClientConfig dbConfig;
		dbConfig.addTargetServer(svInfo);
	} 

	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		TriggerInfo *trigInfo = &testTriggerInfo[i];
		DBClientAsura dbAsura;
		dbAsura.addTriggerInfo(trigInfo);
	} 

	return EXIT_SUCCESS;
}
