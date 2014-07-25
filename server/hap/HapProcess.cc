/*
 * Copyright (C) 2014 Project Hatohol
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

#include <glib.h>
#include <cstdio>
#include <AtomicValue.h>
#include "HapProcess.h"
#include "HatoholException.h"

using namespace mlpl;

const int HapProcess::DEFAULT_EXCEPTION_SLEEP_TIME_MS = 60 * 1000;

struct HapProcess::PrivateContext {
	GMainLoop *loop;
	AtomicValue<int> exceptionSleepTimeMS;
	ArmStatus        armStatus;
	HapCommandLineArg cmdLineArg;
	GError           *cmdLineArgParseError;

	PrivateContext(void)
	: loop(NULL),
	  exceptionSleepTimeMS(DEFAULT_EXCEPTION_SLEEP_TIME_MS),
	  cmdLineArgParseError(NULL)
	{
	}

	~PrivateContext()
	{
		if (loop)
			g_main_loop_unref(loop);
		if (cmdLineArgParseError)
			g_error_free(cmdLineArgParseError);
	}
};

// ---------------------------------------------------------------------------
// HapCommandLineArg
// ---------------------------------------------------------------------------
HapCommandLineArg::HapCommandLineArg(void)
: brokerUrl(NULL),
  queueAddress(NULL)
{
}

HapCommandLineArg::~HapCommandLineArg()
{
	g_free((gchar *)brokerUrl);
	g_free((gchar *)queueAddress);
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapProcess::HapProcess(int argc, char *argv[])
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	parseCommandLineArg(m_ctx->cmdLineArg, argc, argv);
}

HapProcess::~HapProcess()
{
	delete m_ctx;
}

void HapProcess::initGLib(void)
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
#ifndef GLIB_VERSION_2_32
	g_thread_init(NULL);
#endif // GLIB_VERSION_2_32
}

GMainLoop *HapProcess::getGMainLoop(void)
{
	if (!m_ctx->loop)
		m_ctx->loop = g_main_loop_new(NULL, FALSE);
	HATOHOL_ASSERT(m_ctx->loop, "Failed to create GMainLoop.");
	return m_ctx->loop;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ArmStatus &HapProcess::getArmStatus(void)
{
	return m_ctx->armStatus;
}

void HapProcess::parseCommandLineArg(
  HapCommandLineArg &arg, int argc, char *argv[])
{
	GOptionEntry entries[] = {
		{"broker-url", 'b', 0, G_OPTION_ARG_STRING,
		 &arg.brokerUrl, "Broker URL", "URL:PORT"},
		{"queue-address", 'q', 0, G_OPTION_ARG_STRING,
		 &arg.queueAddress, "Queue Address", "ADDR"},
		{NULL}
	};

	GOptionContext *context;
	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv,
	                       &m_ctx->cmdLineArgParseError);
	g_option_context_free(context);
}

const GError *HapProcess::getErrorOfCommandLineArg(void) const
{
	return m_ctx->cmdLineArgParseError;
}

const HapCommandLineArg &HapProcess::getCommandLineArg(void) const
{
	return m_ctx->cmdLineArg;
}
