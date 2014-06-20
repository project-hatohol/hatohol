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
#include "HapProcess.h"
#include "HatoholException.h"

struct HapProcess::PrivateContext {
	GMainLoop *loop;

	PrivateContext(void)
	: loop(NULL)
	{
	}

	~PrivateContext()
	{
		if (loop)
			g_main_loop_unref(loop);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapProcess::HapProcess(int argc, char *argv[])
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

HapProcess::~HapProcess()
{
	if (m_ctx)
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
