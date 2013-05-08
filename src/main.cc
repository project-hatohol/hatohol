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
#include <cstring>
#include <glib.h>
#include <errno.h>

#include <string>
#include <vector>
using namespace std;

#include <Logger.h>
using namespace mlpl;

#include "Asura.h"
#include "Utils.h"
#include "FaceMySQL.h"
#include "FaceRest.h"
#include "VirtualDataStoreZabbix.h"
#include "DBClientConfig.h"

static int pipefd[2];

struct ExecContext {
	GMainLoop *loop;
};

static void signalHandlerToExit(int signo, siginfo_t *info, void *arg)
{
	// We use close() to notify the exit request from a signal handler,
	// because close() is one of the asynchronus SIGNAL safe functions.
	close(pipefd[1]);
}

static void setupSignalHandlerForExit(int signo)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_sigaction = signalHandlerToExit;
	sa.sa_flags |= (SA_RESTART|SA_SIGINFO);
	ASURA_ASSERT(sigaction(signo, &sa, NULL ) == 0,
	             "Failed to set SIGNAL: %d, errno: %d\n", signo, errno);
}

gboolean exitFunc(GIOChannel *source, GIOCondition condition, gpointer data)
{
	MLPL_INFO("recieved stop request.\n");
	ExecContext *ctx = static_cast<ExecContext *>(data);

	// TODO: to shoutdown servers

	// Because this function is beeing called, ctx->loop must have valid
	// value even if a signal is received before ctx->loop is created.
	g_main_loop_quit(ctx->loop);

	return FALSE;
}

static void setupGizmoForExit(gpointer data)
{
	// open a pipe to communicate with the main thread
	ASURA_ASSERT(pipe(pipefd) == 0,
		     "Failed to open pipe: errno: %d", errno);

	GIOChannel *ioch = g_io_channel_unix_new(pipefd[0]);
	g_io_add_watch(ioch, G_IO_HUP, exitFunc, data);
}

int mainRoutine(int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
	asuraInit();
	MLPL_INFO("started asura: ver. %s\n", PACKAGE_VERSION);

	// setup signal handlers for exit
	ExecContext ctx;
	setupGizmoForExit(&ctx);
	setupSignalHandlerForExit(SIGTERM);

	// parse command line arguemnt
	CommandLineArg cmdArg;
	for (int i = 1; i < argc; i++)
		cmdArg.push_back(argv[i]);

	// setup configuration database
	DBClientConfig::parseCommandLineArgument(cmdArg);
	DBClientConfig dbConfig;
	if (dbConfig.isFaceMySQLEnabled()) {
		FaceMySQL face(cmdArg);
		face.start();
	}

	// start REST server
	FaceRest rest(cmdArg);
	rest.start();

	// start VirtualDataStoreZabbix
	VirtualDataStoreZabbix *vdsZabbix
	  = VirtualDataStoreZabbix::getInstance();
	vdsZabbix->start();

	// main loop of GLIB
	ctx.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ctx.loop);

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	try {
		ret = mainRoutine(argc, argv);
	} catch (const AsuraException &e){
		MLPL_ERR("Got exception: %s", e.getFancyMessage().c_str());
	} catch (const exception &e) {
		MLPL_ERR("Got exception: %s", e.what());
	}
	return ret;
}
