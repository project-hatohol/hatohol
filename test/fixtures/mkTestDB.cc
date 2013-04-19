#include <cstdio>
#include <cstdlib>
#include "Asura.h"
#include "DBAgentTest.h"
#include "DBAgentSQLite3.h"
#include "DBClientAsura.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Need one argument for the DB name.");
		return EXIT_FAILURE;
	}

	string dbName(argv[1]);
	printf("DBName: %s\n", dbName.c_str());

	asuraInit();
	DBAgentSQLite3::defineDBPath(DefaultDBDomainId, dbName);

	DBClientAsura dbAsura;
	for (size_t i = 0; i < NumServerInfo; i++) {
		MonitoringServerInfo *svInfo = &serverInfo[i];
		dbAsura.addTargetServer(svInfo);
	} 

	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		TriggerInfo *trigInfo = &testTriggerInfo[i];
		dbAsura.addTriggerInfo(trigInfo);
	} 

	return EXIT_SUCCESS;
}
