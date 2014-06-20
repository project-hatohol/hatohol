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

#include <HapZabbixAPI.h>
#include "HapProcess.h"

class HapProcessZabbixAPI : public HapProcess, public HapZabbixAPI {
public:
	HapProcessZabbixAPI(int argc, char *argv[]);
	int mainLoopRun(void);

protected:
	gpointer hapMainThread(HatoholThreadArg *arg) override;
};

HapProcessZabbixAPI::HapProcessZabbixAPI(int argc, char *argv[])
: HapProcess(argc, argv)
{
	initGLib();
}

int HapProcessZabbixAPI::mainLoopRun(void)
{
	g_main_loop_run(getGMainLoop());
	return EXIT_SUCCESS;
}

gpointer HapProcessZabbixAPI::hapMainThread(HatoholThreadArg *arg)
{
	return NULL;
}

int main(int argc, char *argv[])
{
	HapProcessZabbixAPI hapProc(argc, argv);
	return hapProc.mainLoopRun();
}
