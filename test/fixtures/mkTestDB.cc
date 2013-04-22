#include <cstdio>
#include <cstdlib>
#include "Asura.h"
#include "DBClientTest.h"
#include "DBAgentSQLite3.h"
#include "DBClientAsura.h"
#include "ConfigManager.h"

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Need one argument for the DB name.");
		return EXIT_FAILURE;
	}

	string dbNameConfig(argv[1]);
	string dbNameAsura(argv[2]);
	printf("DBName (config): %s\n", dbNameConfig.c_str());
	printf("DBName (asura) : %s\n", dbNameAsura.c_str());

	asuraInit();
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_CONFIG, dbNameConfig);
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_ASURA, dbNameAsura);

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
