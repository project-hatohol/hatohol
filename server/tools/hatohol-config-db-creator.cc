/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <glib-object.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include "Hatohol.h"
#include "DBClientConfig.h"
#include "ConfigManager.h"
#include "DBAgentMySQL.h"

using namespace std;

struct ConfigValue {
	string                   configDBServer;
	int                      configDBServerPort;
	string                   configDBName;
	string                   configDBUser;
	string                   configDBPassword;
	int                      faceRestPort;
	MonitoringServerInfoList serverInfoList;
	
	// constructor
	ConfigValue(void)
	: configDBServerPort(0),
	  configDBName("hatohol"),
	  configDBUser("hatohol"),
	  configDBPassword("hatohol"),
	  faceRestPort(0)
	{
	}
};

static void printUsage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "$ hatohol-config-db-creator config.dat\n");
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

static bool parseInt(ParsableString &parsable, int &intVal, size_t lineNo)
{
	string word = parsable.readWord(ParsableString::SEPARATOR_COMMA);
	intVal = atoi(word.c_str());
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

	// user name
	if (!extractString(parsable, word, lineNo))
		return false;
	serverInfo.userName = word;

	// passowrd
	if (!extractString(parsable, word, lineNo))
		return false;
	serverInfo.password = word;

	// dbName
	if (serverInfo.type == MONITORING_SYSTEM_NAGIOS) {
		if (!extractString(parsable, word, lineNo))
			return false;
		serverInfo.dbName = word;
	}

	// push back the info
	serverInfoList.push_back(serverInfo);
	return true;
}

static bool readConfigFile(const string &configFilePath, ConfigValue &confValue)
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
		bool succeeded = true;
		if (element == "configDBServer") {
			succeeded = extractString(parsable,
			                          confValue.configDBServer,
			                          lineNo);
		} else if (element == "configDBServerPort") {
			succeeded = parseInt(parsable,
			                     confValue.configDBServerPort,
			                     lineNo);
		} else if (element == "faceRestPort") {
			succeeded =
			   parseInt(parsable, confValue.faceRestPort, lineNo);
		} else if (element == "server") {
			if (!parseServerConfigLine
			       (parsable, confValue.serverInfoList, lineNo))
				return false;
		} else {
			fprintf(stderr, "Unknown element: %zd: %s\n",
			        lineNo, element.c_str());
			return false;
		}
		if (!succeeded)
			return false;
	}

	return true;
}

static bool validatePort(int port)
{
	// port
	if (port < 0 || port > 65535) {
		fprintf(stderr, "Invalid port: %d\n", port);
		return false;
	}
	return true;
}

static bool validateServerInfoList(ConfigValue &confValue)
{
	set<int> serverIds;
	MonitoringServerInfoListIterator it = confValue.serverInfoList.begin();
	for (; it != confValue.serverInfoList.end(); ++it) {
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
		if (!validatePort(svInfo.port))
			return false;

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

static bool setEchoBack(bool enable)
{
	int fd = open("/dev/tty", O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "Failed to open /dev/tty: %d\n", errno);
		return false;
	}

	struct termios tmios;
	if (tcgetattr(fd, &tmios) == -1) {
		fprintf(stderr, "Failed to call tcgetattr: %d\n", errno);
		close(fd);
		return false;
	}

	if (enable)
		tmios.c_lflag |= ECHO; 
	else
		tmios.c_lflag &= ~ECHO; 

	if (tcsetattr(fd, TCSANOW, &tmios) == -1) {
		fprintf(stderr, "Failed to call tcsetattr: %d\n", errno);
		close(fd);
		return false;
	}

	close(fd);
	return true;
}

static bool execCommand(const string cmd, string &stdoutStr, string &stderrStr)
{
	gboolean ret;
	gchar *standardOutput = NULL;
	gchar *standardError = NULL;
	gint exitStatus;
	GError *error = NULL;
	string errorStr;

	ret = g_spawn_command_line_sync(cmd.c_str(),
	                                &standardOutput, &standardError,
	                                &exitStatus, &error);
	if (standardOutput) {
		stdoutStr = standardOutput;
		g_free(standardOutput);
	}
	if (standardError) {
		stderrStr = standardError;
		g_free(standardError);
	}
	if (!ret)
		return false;
	if (exitStatus != 0)
		return false;
	return true;
}

static void showCommandError(const string &stdoutStr, const string &stderrStr)
{
	fprintf(stderr,
	        "Failed to execute command:\n"
	        "<<stdout>>\n"
	        "%s\n"
	        "<<stderr>>\n"
	        "%s\n",
	        stdoutStr.c_str(), stderrStr.c_str());
}

static bool execMySQL(const string &statement,
                      const string &user, const string &passwd,
                      string &stdoutStr)
{
	string passwdStr;
	if (!passwd.empty())
		passwdStr = "-p" + passwd;

	string cmd = StringUtils::sprintf(
	  "mysql -u %s %s -N -B -e \"%s\"", 
	  user.c_str(), passwdStr.c_str(), statement.c_str());
	string stderrStr;
	if (!execCommand(cmd, stdoutStr, stderrStr)) {
		showCommandError(stdoutStr, stderrStr);
		return false;
	}
	return true;
}

static bool dropConfigDBIfExists(const ConfigValue &confValue,
                                 const string &dbRootPasswd)
{
	string stdoutStr;
	string sql;

	// check if the DB exists
	sql = StringUtils::sprintf(
	  "SHOW DATABASES LIKE '%s'", confValue.configDBName.c_str());
	if (!execMySQL(sql, "root", dbRootPasswd, stdoutStr))
		return false;

	StringVector strVect;
	StringUtils::split(strVect, stdoutStr, '\n');
	if (strVect.empty())
		return true;

	printf("The database: %s will be dropped. Are you OK? (yes/no)\n:",
	       confValue.configDBName.c_str());
	string reply;
	while (true) {
		cin >> reply;
		printf("\n");
		if (reply == "yes")
			break;
		if (reply == "no") {
			printf("*** The setup was canncelled ***\n");
			return false;
		}
		printf("You should type yes or no :");
	}

	// drop the DB
	sql = StringUtils::sprintf(
	  "DROP DATABASE %s", confValue.configDBName.c_str());
	if (!execMySQL(sql, "root", dbRootPasswd, stdoutStr))
		return false;

	return true;
}

static bool createConfigDB(const ConfigValue &confValue,
                           const string &dbRootPasswd)
{
	string stdoutStr;
	string sql = StringUtils::sprintf(
	  "CREATE DATABASE %s", confValue.configDBName.c_str());
	if (!execMySQL(sql, "root", dbRootPasswd, stdoutStr))
		return false;
	return true;
}

static bool setGrant(const ConfigValue &confValue,
                     const string &dbRootPasswd)
{
	string stdoutStr;
	string sql = StringUtils::sprintf(
	  "GRANT ALL ON %s.* TO %s@'%%' IDENTIFIED BY '%s'",
	   confValue.configDBName.c_str(),
	   confValue.configDBUser.c_str(),
	   confValue.configDBPassword.c_str());
	if (!execMySQL(sql, "root", dbRootPasswd, stdoutStr))
		return false;

	if (!execMySQL("FLUSH PRIVILEGES", "root", dbRootPasswd, stdoutStr))
		return false;

	return true;
}

static bool setupDBServer(const ConfigValue &confValue)
{
	string confDBPortStr =
	   confValue.configDBServerPort == 0 ?
	     "(default)" :
	     StringUtils::sprintf("%d", confValue.configDBServerPort); 
	printf("Configuration DB Server: '%s', port: %s\n",
	       confValue.configDBServer.c_str(), confDBPortStr.c_str());
	printf("\n"
	       "Please input the root password for the above server.\""
	       "(If you've not set the root password, just push enter)\n"
	       ":");
	if (!setEchoBack(false))
		return false;
	string dbRootPasswd;
	while (int ch = getchar()) {
		if (ch == '\n')
			break;
		dbRootPasswd += ch;
	}
	if (!setEchoBack(true))
		return false;
	printf("\n");

	if (!dropConfigDBIfExists(confValue, dbRootPasswd))
		return false;
	if (!createConfigDB(confValue, dbRootPasswd))
		return false;
	if (!setGrant(confValue, dbRootPasswd))
		return false;

	return true;
}

int main(int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
	hatoholInit();

	if (argc < 2) {
		printUsage();
		return EXIT_FAILURE;
	}

	const string configFilePath = argv[1];

	// opening config.dat and read it
	ConfigValue confValue;
	if (!readConfigFile(configFilePath, confValue))
		return EXIT_FAILURE;

	// validation
	if (!validatePort(confValue.configDBServerPort))
		return EXIT_FAILURE;
	if (!validatePort(confValue.faceRestPort))
		return EXIT_FAILURE;
	if (!validateServerInfoList(confValue))
		return EXIT_FAILURE;

	//
	// Setup the DB.
	//
	if (!setupDBServer(confValue))
		return EXIT_SUCCESS;
	DBClientConfig dbConfig;

	// FaceRest port
	dbConfig.setFaceRestPort(confValue.faceRestPort);
	printf("FaceRest port: %d\n", confValue.faceRestPort);

	// servers
	MonitoringServerInfoListIterator it = confValue.serverInfoList.begin();
	for (; it != confValue.serverInfoList.end(); ++it) {
		MonitoringServerInfo &svInfo = *it;
		dbConfig.addTargetServer(&svInfo);

		printf("SERVER: ID: %d, TYPE: %d, HOSTNAME: %s, IP ADDR: %s "
		       "NICKNAME: %s, PORT: %d, POLLING: %d, RETRY: %d "
		       "USERNAME: %s, PASSWORD: %s, DB NAME: %s\n",
		       svInfo.id, svInfo.type, svInfo.hostName.c_str(),
		       svInfo.ipAddress.c_str(), svInfo.nickname.c_str(),
		       svInfo.port, svInfo.pollingIntervalSec,
		       svInfo.retryIntervalSec,
		       svInfo.userName.c_str(), svInfo.password.c_str(),
		       svInfo.dbName.c_str());
	}

	return EXIT_SUCCESS;
}
