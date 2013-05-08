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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <glib-object.h>
#include "Asura.h"
#include "DBClientConfig.h"
#include "ConfigManager.h"
#include "DBAgentSQLite3.h"

using namespace std;

static void printUsage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "$ asura-config-db-creator config.dat dbfile.db\n");
	fprintf(stderr, "\n");
}

bool checkUnexpectedFinish(ParsableString &parsable, size_t lineNo)
{
	if (parsable.finished()) {
		fprintf(stderr, "Unexpectedly line finished: %zd: %s\n",
		        lineNo, parsable.getString());
		return false;
	}
	return true;
}

static bool extractString(ParsableString &parsable, string &str, size_t lineNo)
{
	string word = parsable.readWord(ParsableString::SEPARATOR_COMMA);
	str = StringUtils::stripBothEndsSpaces(word);
	return true;
}

static bool parseServerConfigLine(ParsableString &parsable,
                                  MonitoringServerInfoList &serverInfoList,
                                  size_t lineNo)
{
	string word;
	MonitoringServerInfo serverInfo;

	// ID
	word = parsable.readWord(ParsableString::SEPARATOR_COMMA);
	if (!checkUnexpectedFinish(parsable, lineNo))
		return false;
	serverInfo.id = atoi(word.c_str());

	// Type
	word = parsable.readWord(ParsableString::SEPARATOR_COMMA);
	if (!checkUnexpectedFinish(parsable, lineNo))
		return false;
	serverInfo.type = (MonitoringSystemType)atoi(word.c_str());

	// Hostname
	if (!extractString(parsable, word, lineNo))
		return false;
	serverInfo.hostName = word;

	// IP address
	if (!extractString(parsable, word, lineNo))
		return false;
	serverInfo.ipAddress = word;

	// Nickname
	if (!extractString(parsable, word, lineNo))
		return false;
	serverInfo.nickname = word;

	// Port
	word = parsable.readWord(ParsableString::SEPARATOR_COMMA);
	if (!checkUnexpectedFinish(parsable, lineNo))
		return false;
	serverInfo.port = atoi(word.c_str());

	// Polling Interval
	word = parsable.readWord(ParsableString::SEPARATOR_COMMA);
	if (!checkUnexpectedFinish(parsable, lineNo))
		return false;
	serverInfo.pollingIntervalSec = atoi(word.c_str());

	// Retry Interval
	word = parsable.readWord(ParsableString::SEPARATOR_COMMA);
	serverInfo.retryIntervalSec = atoi(word.c_str());

	// push back the info
	serverInfoList.push_back(serverInfo);
	return true;
}

static bool readConfigFile(const string &configFilePath,
                           MonitoringServerInfoList &serverInfoList)
{
	ifstream ifs(configFilePath.c_str());
	if (!ifs) {
		fprintf(stderr, "Failed to open file: %s\n",
		        configFilePath.c_str());
		return false;
	}

	string line;
	size_t lineNo = 0;
	MonitoringServerInfo serverInfo;
	while (getline(ifs, line)) {
		lineNo++;
		string strippedLine = StringUtils::stripBothEndsSpaces(line);

		// skip empty line
		if (strippedLine.empty())
			continue;

		// skip comment line
		if (strippedLine[0] == '#')
			continue;

		// try to find colon
		SeparatorChecker separator(":");
		ParsableString parsable(strippedLine);
		string element = parsable.readWord(separator);
		if (!checkUnexpectedFinish(parsable, lineNo))
			return false;

		// dispatch
		if (element == "server") {
			if (!parseServerConfigLine(parsable, serverInfoList,
			                           lineNo))
				return false;
		} else {
			fprintf(stderr, "Unknown element: %zd: %s\n",
			        lineNo, element.c_str());
			return false;
		}
	}

	return true;
}

static bool validateServerInfoList(MonitoringServerInfoList &serverInfoList)
{
	set<int> serverIds;
	MonitoringServerInfoListIterator it = serverInfoList.begin();
	for (; it != serverInfoList.end(); ++it) {
		// ID (Don't be duplicative)
		MonitoringServerInfo &svInfo = *it;
		if (serverIds.find(svInfo.id) != serverIds.end()) {
			fprintf(stderr, "Duplicated server ID: %d\n",
			        svInfo.id);
			return false;
		}
		serverIds.insert(svInfo.id);

		// should be given hostname or IP address.
		if (svInfo.hostName.empty() && svInfo.ipAddress.empty()) {
			fprintf(stderr,
			        "Sould be specify hostname or IP adress.\n");
		}

		// TODO: If IP address is given, check the format

		// port
		if (svInfo.port < 0 || svInfo.port > 65535) {
			fprintf(stderr, "Invalid port: %d\n", svInfo.port);
			return false;
		}

		// polling interval
		if (svInfo.pollingIntervalSec < 0) {
			fprintf(stderr, "Invalid polling interval: %d\n",
			        svInfo.pollingIntervalSec);
			return false;
		}

		// retry interval
		if (svInfo.retryIntervalSec < 0) {
			fprintf(stderr, "Invalid retry interval: %d\n",
			        svInfo.retryIntervalSec);
			return false;
		}
	}
	
	return true;
}

int main(int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
	asuraInit();

	if (argc < 3) {
		printUsage();
		return EXIT_FAILURE;
	}

	const string configFilePath = argv[1];
	const string configDBPath = argv[2];

	// opening config.dat and read it
	MonitoringServerInfoList serverInfoList;
	if (!readConfigFile(configFilePath, serverInfoList))
		return EXIT_FAILURE;

	// validation
	if (!validateServerInfoList(serverInfoList))
		return EXIT_FAILURE;

	// Save data to DB.
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_CONFIG, configDBPath);
	DBClientConfig dbConfig;
	MonitoringServerInfoListIterator it = serverInfoList.begin();
	for (; it != serverInfoList.end(); ++it) {
		MonitoringServerInfo &svInfo = *it;
		dbConfig.addTargetServer(&svInfo);

		printf("SERVER: ID: %d, TYPE: %d, HOSTNAME: %s, IP ADDR: %s "
		       "NICKNAME: %s, PORT: %d, POLLING: %d, RETRY: %d\n",
		       svInfo.id, svInfo.type, svInfo.hostName.c_str(),
		       svInfo.ipAddress.c_str(), svInfo.nickname.c_str(),
		       svInfo.port, svInfo.pollingIntervalSec,
		       svInfo.retryIntervalSec);
	}

	return EXIT_SUCCESS;
}
