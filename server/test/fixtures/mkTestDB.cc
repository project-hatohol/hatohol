#include <glib-object.h>
#include <cstdio>
#include <cstdlib>
#include "Hatohol.h"
#include "DBClientTest.h"
#include "DBAgentSQLite3.h"
#include "DBClientHatohol.h"
#include "ConfigManager.h"

typedef void (*DBMaker)(const string &dbName);

static void makeDBConfig(const string &dbName)
{
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_CONFIG, dbName);
	for (size_t i = 0; i < NumServerInfo; i++) {
		MonitoringServerInfo *svInfo = &serverInfo[i];
		DBClientConfig dbConfig;
		dbConfig.addTargetServer(svInfo);
	} 
}

static void makeDBHatohol(const string &dbName)
{
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_HATOHOL, dbName);
	DBClientHatohol dbHatohol;

	// Triggers
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		TriggerInfo *trigInfo = &testTriggerInfo[i];
		dbHatohol.addTriggerInfo(trigInfo);
	} 

	// Events
	for (size_t i = 0; i < NumTestEventInfo; i++) {
		EventInfo *eventInfo = &testEventInfo[i];
		dbHatohol.addEventInfo(eventInfo);
	} 

	// Items
	for (size_t i = 0; i < NumTestItemInfo; i++) {
		ItemInfo *itemInfo = &testItemInfo[i];
		dbHatohol.addItemInfo(itemInfo);
	} 
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Need one argument for the DB name.");
		return EXIT_FAILURE;
	}

	string command(argv[1]);
	string dbName(argv[2]);

	printf("command: %s\n", command.c_str());
	printf("DBName : %s\n", dbName.c_str());

#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
	hatoholInit();
	DBMaker dbMaker = NULL;
	if (command == "config")
		dbMaker = makeDBConfig;
	else if (command == "hatohol")
		dbMaker = makeDBHatohol;

	if (!dbMaker) {
		printf("Unknwon command\n");
		return EXIT_FAILURE;
	}

	(*dbMaker)(dbName);

	return EXIT_SUCCESS;
}
