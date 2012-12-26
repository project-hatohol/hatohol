#include <cstdio>
#include <cstdlib>
#include <glib.h>

#include <string>
#include <vector>
using namespace std;

#include "utils.h"
#include "face-mysql.h"

int main(int argc, char *argv[])
{
	g_type_init();
	ASURA_P(INFO, "started asura: ver. %s\n", PACKAGE_VERSION);

	command_line_arg_t cmd_arg;
	for (int i = 1; i < argc; i++)
		cmd_arg.push_back(argv[i]);

	face_mysql face(cmd_arg);
	face.start();
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	return EXIT_SUCCESS;
}
