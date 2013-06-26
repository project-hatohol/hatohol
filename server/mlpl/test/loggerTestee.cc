#include <stdio.h>
#include <stdlib.h>
#include "Logger.h"
using namespace mlpl;

#include <string>
using namespace std;

#include "loggerTester.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "argc < 2\n");
		return EXIT_FAILURE;
	}
	string level = argv[1];
	if (level == "DBG")
		MLPL_DBG("%s\n", testString);
	else if (level == "INFO")
		MLPL_INFO("%s\n", testString);
	else if (level == "WARN")
		MLPL_WARN("%s\n", testString);
	else if (level == "ERR")
		MLPL_ERR("%s\n", testString);
	else if (level == "CRIT")
		MLPL_CRIT("%s\n", testString);
	else if (level == "BUG")
		MLPL_BUG("%s\n", testString);
	else {
		fprintf(stderr, "unknown level: %s\n", level.c_str());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
