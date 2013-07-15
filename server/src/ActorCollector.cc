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

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <cstring>
#include "ActorCollector.h"
#include "DBClientAction.h"
#include "Logger.h"

using namespace mlpl;

struct ExitChildInfo {
	pid_t pid;
	int   status;
	int   exitCode;
};

struct ActorCollector::PrivateContext {
	static int     pipefd[2];
	DBClientAction dbAction;
};

int ActorCollector::PrivateContext::pipefd[2];

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ActorCollector::init(void)
{
	setupHandlerForSIGCHLD();
}

void ActorCollector::stop(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

ActorCollector::ActorCollector(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ActorCollector::~ActorCollector()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ActorCollector::setupHandlerForSIGCHLD(void)
{
	// open pipe
	HATOHOL_ASSERT(pipe(PrivateContext::pipefd) == 0,
	               "Failed to open pipe: errno: %d", errno);

	// set signal handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_sigaction = ActorCollector::signalHandlerChild;
	sa.sa_flags |= (SA_RESTART|SA_SIGINFO);
	HATOHOL_ASSERT(sigaction(SIGCHLD, &sa, NULL ) == 0,
	               "Failed to set SIGCHLD, errno: %d\n", errno);
	
	// set glib callback handler for the pipe
	GIOChannel *ioch = g_io_channel_unix_new(PrivateContext::pipefd[0]);
	g_io_add_watch(ioch,
	               (GIOCondition)(G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP),
	               ActorCollector::checkExitProcess, NULL);
}

void ActorCollector::signalHandlerChild(int signo, siginfo_t *info, void *arg)
{
	if (info->si_code != CLD_EXITED)
		return;

	// We use write() to notify the reception of SIGCHLD.
	// because write() is one of the asynchronus SIGNAL safe functions.
	ExitChildInfo exitChildInfo;
	exitChildInfo.pid      = info->si_pid;
	exitChildInfo.status   = info->si_status;
	exitChildInfo.exitCode = info->si_stime;
	ssize_t ret = write(PrivateContext::pipefd[1],
	                    &exitChildInfo, sizeof(ExitChildInfo));
	if (ret == -1) {
		// TODO: What should we do ?
	}
}

gboolean ActorCollector::checkExitProcess
  (GIOChannel *source, GIOCondition condition, gpointer data)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return TRUE;
}
