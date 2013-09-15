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

#include <cstdio>
#include <cstdlib>

#include "Hatohol.h"
#include "ArmZabbixAPI.h"

static const int DEFAULT_PORT = 80;
static const int ZBX_SVR_ID = 0;

class ZabbixAPIResponseCollector : public ArmZabbixAPI
{
	typedef bool (ZabbixAPIResponseCollector::*CommandFunc)
	               (const string &command, vector<string>& cmdArgs);
	typedef map<string, CommandFunc> CommandFuncMap;
	typedef CommandFuncMap::iterator CommandFuncMapIterator;

	CommandFuncMap m_commandFuncMap;
public:
	ZabbixAPIResponseCollector(const MonitoringServerInfo &serverInfo);
	virtual ~ZabbixAPIResponseCollector();
	bool execute(const string &command, vector<string> &cmdArg);

protected:
	bool commandFuncTrigger(const string &command, vector<string>& cmdArgs);
	bool commandFuncItem(const string &command, vector<string>& cmdArgs);
	bool commandFuncHost(const string &command, vector<string>& cmdArgs);
	bool commandFuncEvent(const string &command, vector<string>& cmdArgs);
	bool commandFuncApplication(const string &command, vector<string>& cmdArgs);
	bool commandFuncOpen(const string &command, vector<string>& cmdArgs);
};

ZabbixAPIResponseCollector::ZabbixAPIResponseCollector
  (const MonitoringServerInfo &serverInfo)
: ArmZabbixAPI(serverInfo)
{
	m_commandFuncMap["open"] = 
	  &ZabbixAPIResponseCollector::commandFuncOpen;
	m_commandFuncMap["trigger"] = 
	  &ZabbixAPIResponseCollector::commandFuncTrigger;
	m_commandFuncMap["item"] = 
	  &ZabbixAPIResponseCollector::commandFuncItem;
	m_commandFuncMap["host"] = 
	  &ZabbixAPIResponseCollector::commandFuncHost;
	m_commandFuncMap["event"] = 
	  &ZabbixAPIResponseCollector::commandFuncEvent;
	m_commandFuncMap["application"] = 
	  &ZabbixAPIResponseCollector::commandFuncApplication;
}

ZabbixAPIResponseCollector::~ZabbixAPIResponseCollector()
{
}

bool ZabbixAPIResponseCollector::execute(const string &command,
                                         vector<string> &cmdArgs)
{
	CommandFuncMapIterator it = m_commandFuncMap.find(command);
	if (it == m_commandFuncMap.end()) {
		fprintf(stderr, "Unknwon command: %s\n", command.c_str());
		return false;
	}
	CommandFunc func = it->second;
	return (this->*func)(command, cmdArgs);
}

bool ZabbixAPIResponseCollector::commandFuncOpen
  (const string &command, vector<string>& cmdArgs)
{
	SoupMessage *msg;
	if (!openSession(&msg))
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncTrigger
  (const string &command, vector<string>& cmdArgs)
{
	if (!commandFuncOpen(command, cmdArgs))
		return false;

	SoupMessage *msg = queryTrigger();
	if (!msg)
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncItem
  (const string &command, vector<string>& cmdArgs)
{
	if (!commandFuncOpen(command, cmdArgs))
		return false;

	SoupMessage *msg = queryItem();
	if (!msg)
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncHost
  (const string &command, vector<string>& cmdArgs)
{
	if (!commandFuncOpen(command, cmdArgs))
		return false;

	vector<uint64_t> hostIds; // empty means all hosts.
	SoupMessage *msg = queryHost(hostIds);
	if (!msg)
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncEvent
  (const string &command, vector<string>& cmdArgs)
{
	if (!commandFuncOpen(command, cmdArgs))
		return false;

	SoupMessage *msg = queryEvent(0);
	if (!msg)
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncApplication
  (const string &command, vector<string>& cmdArgs)
{
	if (!commandFuncOpen(command, cmdArgs))
		return false;

	vector<uint64_t> hostIds; // empty means all hosts.
	SoupMessage *msg = queryApplication(hostIds);
	if (!msg)
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

static void printUsage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "$ zabbix-api-response-collector server commad\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "command:\n");
	fprintf(stderr, "  open\n");
	fprintf(stderr, "  trigger\n");
	fprintf(stderr, "  item\n");
	fprintf(stderr, "  host\n");
	fprintf(stderr, "  event\n");
	fprintf(stderr, "  application\n");
	fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
	hatoholInit();
	if (argc < 3) {
		printUsage();
		return EXIT_FAILURE;
	}
	string server = argv[1];
	string command = argv[2];
	vector<string> cmdArgs;
	for (int i = 3; i < argc; i++)
		cmdArgs.push_back(argv[i]);

	static MonitoringServerInfo serverInfo = 
	{
		ZBX_SVR_ID,               // id
		MONITORING_SYSTEM_ZABBIX, // type
		"",                       // hostname
		"",                       // ip_address
		"",                       // nickname
		DEFAULT_PORT,             // port
		10,                       // polling_interval_sec
		5,                        // retry_interval_sec

		"admin",                  // userName
		"zabbix",                 // password
	};
	serverInfo.hostName = server;

	ZabbixAPIResponseCollector collector(serverInfo);
	if (!collector.execute(command, cmdArgs))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
