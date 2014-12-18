/*
 * Copyright (C) 2014 Project Hatohol
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

#ifndef HapProcess_h
#define HapProcess_h

#include <glib.h>
#include <glib-object.h>
#include "ArmStatus.h"

struct HapCommandLineArg {
	const gchar *brokerUrl;
	const gchar *queueAddress;

	HapCommandLineArg(void);
	virtual ~HapCommandLineArg();
};

class HapProcess {
public:
	HapProcess(int argc, char *argv[]);
	virtual ~HapProcess();

	/**
	 * Call g_type_init() and g_thread_init if needed.
	 */
	void initGLib(void);

	GMainLoop *getGMainLoop(void);

protected:
	static const int DEFAULT_EXCEPTION_SLEEP_TIME_MS;

	/**
	 * Pasrse the command line argument. This method is called in the
	 * constructor. Generally it's not needed to call explicitly.
	 * The parsing result can be obtained by
	 * getErrorOfCommandLineArg() and getCommandLineArg().
	 */
	void parseCommandLineArg(HapCommandLineArg &arg,
	                         int argc, char *argv[]);
	/**
	 * Get the error status of the command line argument parsing.
	 *
	 * @return
	 * A GError object. If no errors happen, NULL is returned.
	 * The returned object must not be freed.
	 */
	const GError *getErrorOfCommandLineArg(void) const;
	const HapCommandLineArg &getCommandLineArg(void) const;

	ArmStatus &getArmStatus(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // HapProcess_h
