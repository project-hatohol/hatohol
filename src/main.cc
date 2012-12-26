#include <cstdio>
#include <cstdlib>
#include <glib.h>
#include "face-mysql.h"

int main(int argc, char *argv[])
{
	face_mysql face;
	face.start();
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	return EXIT_SUCCESS;
}
