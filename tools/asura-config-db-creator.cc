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

using namespace std;

static void printUsage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "$ asura-config-db-creator config.dat dbfile.db\n");
	fprintf(stderr, "\n");
}

static bool parseServerConfigLine(const string &buf,
                                  MonitoringServerInfo &serverInfo)
{
	// TODO: implemented
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

	string buf;
	while (getline(ifs, buf)) {
		MonitoringServerInfo serverInfo;
		if (!parseServerConfigLine(buf, serverInfo))
			return false;
		serverInfoList.push_back(serverInfo);
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

	// Save data to DB.
	// TODO: implemented

	return EXIT_SUCCESS;
}
