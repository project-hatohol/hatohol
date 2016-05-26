/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>

#include <string>
#include <vector>
using namespace std;

#include <Logger.h>
using namespace mlpl;

#include "Hatohol.h"
#include "Utils.h"
#include "FaceRest.h"
#include "UnifiedDataStore.h"
#include "DBTablesConfig.h"
#include "ActorCollector.h"
#include "DBTablesAction.h"
#include "ConfigManager.h"
#include "ThreadLocalDBCache.h"
#include "ChildProcessManager.h"
#include "ActionManager.h"

static string pidFilePath;
static int pipefd[2];

struct ExecContext {
	UnifiedDataStore *unifiedDataStore;
	GMainLoop *loop;
	CommandLineOptions cmdLineOpts;
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
	HATOHOL_ASSERT(sigaction(signo, &sa, NULL ) == 0,
	             "Failed to set SIGNAL: %d, errno: %d\n", signo, errno);
}

static void exitFuncOnWaitThread(ExecContext *ctx)
{
	MLPL_INFO("start exit process on the dedicated thread.\n");

	ctx->unifiedDataStore->stop();
	DBTablesAction::stop();

	// TODO: implement
	// ChildProcessManager::getInstance()->quit();

	// Because this function is beeing called, ctx->loop must have valid
	// value even if a signal is received before ctx->loop is created.
	g_main_loop_quit(ctx->loop);
}

gboolean exitFunc(GIOChannel *source, GIOCondition condition, gpointer data)
{
	struct ExitFuncThread : public HatoholThreadBase {
		ExecContext *ctx;
		virtual gpointer mainThread(HatoholThreadArg *arg) override
		{
			exitFuncOnWaitThread(ctx);
			return NULL;
		}
	};
	ExitFuncThread *exitFuncThread = new ExitFuncThread();

	MLPL_INFO("recieved stop request.\n");
	exitFuncThread->ctx = static_cast<ExecContext *>(data);
	const bool autoDeleteObject = true;
	exitFuncThread->start(autoDeleteObject);

	return FALSE;
}

static void setupGizmoForExit(gpointer data)
{
	// open a pipe to communicate with the main thread
	HATOHOL_ASSERT(pipe(pipefd) == 0,
		     "Failed to open pipe: errno: %d", errno);

	GIOChannel *ioch = g_io_channel_unix_new(pipefd[0]);
	g_io_add_watch(ioch, G_IO_HUP, exitFunc, data);
	g_io_channel_unref(ioch);
}

static void removePidFile(void)
{
	if (!pidFilePath.length())
		return;
	if (remove(pidFilePath.c_str())) {
		MLPL_ERR("Failed to remove pid file: %s\n",
			 pidFilePath.c_str());
		return;
	}
	pidFilePath.erase();
}

static bool daemonize(void)
{
	pid_t pid;
	pidFilePath = ConfigManager::getInstance()->getPidFilePath();
	FILE *pid_file;
	pid_file = fopen(pidFilePath.c_str(), "w+x");

	if (pid_file == NULL) {
		const char *errfmt;
		if (errno == EEXIST)
			errfmt = "Pid file exists: %s; process already running?\n";
		else
			errfmt = "Failed to record pid file: %s\n";
		MLPL_ERR(errfmt, pidFilePath.c_str());
		pidFilePath.erase();
		return false;
	}

	if (daemon(0, 0) == 0) {
		pid = getpid();
		fprintf(pid_file, "%d\n", pid);
		fclose(pid_file);
		return true;
	} else {
		fclose(pid_file);
		removePidFile();
		pidFilePath.erase();
		return false;
	}
}

static bool checkDBConnection(void)
{
	try {
		ThreadLocalDBCache cache;
		DBHatohol &dbHatohol = cache.getDBHatohol();
		HATOHOL_ASSERT(&dbHatohol,
		  "The reference of DBHatohol is unexpectedly NULL.");
	} catch (const exception &e) {
		MLPL_CRIT("Failed to create a database object. "
		          "This program is aborted. Reason: %s\n",
		          e.what());
		return false;
	}
	return true;
}

static bool changeUser(const string &user)
{
	if (!user.length())
		return true;

	struct passwd *pwd = getpwnam(user.c_str());
	if (!pwd) {
		MLPL_ERR("Failed to get user %s: %s\n",
			 user.c_str(), g_strerror(errno));
		return false;
	}
	if (setuid(pwd->pw_uid)) {
		MLPL_ERR("Failed to switch user to %s (uid=%" PRIuMAX "); %s\n",
			 user.c_str(), (uintmax_t)pwd->pw_uid,
			 g_strerror(errno));
		return false;
	}
	return true;
}

int mainRoutine(int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
#ifndef GLIB_VERSION_2_32
	g_thread_init(NULL);
#endif // GLIB_VERSION_2_32

	// parse command line argument
	ExecContext ctx;
	if (!ConfigManager::parseCommandLine(&argc, &argv, &ctx.cmdLineOpts))
		return EXIT_FAILURE;

	const bool dontCareChildProcessManager = true;
	hatoholInit(&ctx.cmdLineOpts, dontCareChildProcessManager);
	MLPL_INFO("started hatohol server: ver. %s\n", PACKAGE_VERSION);

	if (!checkDBConnection())
		return EXIT_FAILURE;

	ConfigManager *confMgr = ConfigManager::getInstance();

	if (!changeUser(confMgr->getUser()))
		return EXIT_FAILURE;

	if (!confMgr->isForegroundProcess()) {
		if (!daemonize()) {
			MLPL_ERR("Can't start daemon process\n");
			return EXIT_FAILURE;
		}
		hatoholInitChildProcessManager();
	} else {
		pidFilePath.clear();
	}
	hatoholInitChildProcessManager();

	// setup signal handlers for exit
	setupGizmoForExit(&ctx);
	setupSignalHandlerForExit(SIGTERM);
	setupSignalHandlerForExit(SIGINT);
	setupSignalHandlerForExit(SIGHUP);
	setupSignalHandlerForExit(SIGUSR1);

	// setup configuration database
	ThreadLocalDBCache cache;

	// Re execute unfinished action
	ActionManager actionManager;
	actionManager.reExecuteUnfinishedAction();

	// start REST server
	// 'rest' is on a stack. The destructor of it will be automatically
	// called at the end of this function.
	FaceRest rest;
	rest.start();

	ctx.unifiedDataStore = UnifiedDataStore::getInstance();
	ctx.unifiedDataStore->start();

	// main loop of GLIB
	ctx.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ctx.loop);

	g_main_loop_unref(ctx.loop);
	ctx.loop = NULL;
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	try {
		ret = mainRoutine(argc, argv);
	} catch (const HatoholException &e) {
		MLPL_ERR("Got exception: %s", e.getFancyMessage().c_str());
	} catch (const exception &e) {
		MLPL_ERR("Got exception: %s", e.what());
	}
	removePidFile();
	return ret;
}
