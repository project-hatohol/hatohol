#include <cstdio>
#include <cstdlib>

#include "Asura.h"
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
	ZabbixAPIResponseCollector(const string &server,
	                           int port = DEFAULT_PORT);
	virtual ~ZabbixAPIResponseCollector();
	bool execute(const string &command, vector<string> &cmdArg);

protected:
	bool commandFuncTrigger(const string &command, vector<string>& cmdArgs);
};

ZabbixAPIResponseCollector::ZabbixAPIResponseCollector
  (const string &server, int port)
: ArmZabbixAPI(ZBX_SVR_ID, server.c_str(), port)
{
	m_commandFuncMap["trigger"] = 
	  &ZabbixAPIResponseCollector::commandFuncTrigger;
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

bool ZabbixAPIResponseCollector::commandFuncTrigger
  (const string &command, vector<string>& cmdArgs)
{
	if (!openSession())
		return false;

	SoupMessage *msg = queryTrigger();
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
	fprintf(stderr, "  trigger\n");
	fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
	g_type_init();
	asuraInit();
	if (argc < 3) {
		printUsage();
		return EXIT_FAILURE;
	}
	string server = argv[1];
	string command = argv[2];
	vector<string> cmdArgs;
	for (int i = 3; i < argc; i++)
		cmdArgs.push_back(argv[i]);

	ZabbixAPIResponseCollector collector(server);
	if (!collector.execute(command, cmdArgs))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
