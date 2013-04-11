#include <cstdio>
#include <cstdlib>

#include "ArmZabbixAPI.h"

static const int DEFAULT_PORT = 80;
static const int ZBX_SVR_ID = 0;

class ZabbixAPIResponseCollector : public ArmZabbixAPI
{
public:
	ZabbixAPIResponseCollector(const string &server,
	                           int port = DEFAULT_PORT);
	virtual ~ZabbixAPIResponseCollector();
};

ZabbixAPIResponseCollector::ZabbixAPIResponseCollector
  (const string &server, int port)
: ArmZabbixAPI(ZBX_SVR_ID, server.c_str(), port)
{
}

ZabbixAPIResponseCollector::~ZabbixAPIResponseCollector()
{
}

static void printUsage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "$ zabbix-api-response-collector server\n");
	fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printUsage();
		return EXIT_FAILURE;
	}
	string server = argv[1];

	ZabbixAPIResponseCollector collector(server);
	return EXIT_SUCCESS;
}
