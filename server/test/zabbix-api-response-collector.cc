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
#include "HatoholError.h"
using namespace std;

static const int DEFAULT_PORT = 80;
static const int ZBX_SVR_ID = 0;

struct CommandContext {
	const string   &command;
	vector<string> &cmdArgs;
	bool            showOpenInfo;

	CommandContext(const string &_command, vector<string> &_cmdArgs)
	: command(_command),
	  cmdArgs(_cmdArgs),
	  showOpenInfo(true)
	{
	}
};

class ZabbixAPIResponseCollector : public ArmZabbixAPI
{
	typedef bool (ZabbixAPIResponseCollector::*CommandFunc)
	               (CommandContext &ctx);
	typedef map<string, CommandFunc> CommandFuncMap;
	typedef CommandFuncMap::iterator CommandFuncMapIterator;

	CommandFuncMap m_commandFuncMap;
public:
	ZabbixAPIResponseCollector(const MonitoringServerInfo &serverInfo);
	virtual ~ZabbixAPIResponseCollector();
	bool execute(CommandContext &ctx);

protected:
	bool commandFuncTrigger(CommandContext &ctx);
	bool commandFuncItem(CommandContext &ctx);
	bool commandFuncHost(CommandContext &ctx);
	bool commandFuncEvent(CommandContext &ctx);
	bool commandFuncApplication(CommandContext &ctx);
	bool commandFuncOpen(CommandContext &ctx);
	bool commandFuncOpenSilent(CommandContext &ctx);
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

bool ZabbixAPIResponseCollector::execute(CommandContext &ctx)
{
	CommandFuncMapIterator it = m_commandFuncMap.find(ctx.command);
	if (it == m_commandFuncMap.end()) {
		fprintf(stderr, "Unknwon command: %s\n", ctx.command.c_str());
		return false;
	}
	CommandFunc func = it->second;
	return (this->*func)(ctx);
}

bool ZabbixAPIResponseCollector::commandFuncOpen(CommandContext &ctx)
{
	SoupMessage *msg;
	if (!openSession(&msg))
		return false;
	if (ctx.showOpenInfo)
		printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncOpenSilent(CommandContext &ctx)
{
	ctx.showOpenInfo = false;
	return commandFuncOpen(ctx);
}

bool ZabbixAPIResponseCollector::commandFuncTrigger(CommandContext &ctx)
{
	if (!commandFuncOpenSilent(ctx))
		return false;

	HatoholError querRet;
	SoupMessage *msg = queryTrigger(&querRet);
	if (!msg)
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncItem(CommandContext &ctx)
{
	if (!commandFuncOpenSilent(ctx))
		return false;

	HatoholError querRet;
	SoupMessage *msg = queryItem(&querRet);
	if (!msg)
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncHost(CommandContext &ctx)
{
	if (!commandFuncOpenSilent(ctx))
		return false;

	HatoholError querRet;
	SoupMessage *msg = queryHost(&querRet);
	if (!msg)
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncEvent(CommandContext &ctx)
{
	if (!commandFuncOpenSilent(ctx))
		return false;

	HatoholError querRet;
	SoupMessage *msg = queryEvent(0, UNLIMITED,&querRet);
	if (!msg)
		return false;
	printf("%s\n", msg->response_body->data);
	g_object_unref(msg);
	return true;
}

bool ZabbixAPIResponseCollector::commandFuncApplication(CommandContext &ctx)
{
	if (!commandFuncOpenSilent(ctx))
		return false;

	vector<uint64_t> hostIds; // empty means all hosts.
	HatoholError querRet;
	SoupMessage *msg = queryApplication(hostIds,&querRet);
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
	CommandContext ctx(command, cmdArgs);
	if (!collector.execute(ctx))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
