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
#include <glib.h>

#include <string>
#include <vector>
using namespace std;

#include <Logger.h>
using namespace mlpl;

#include "Asura.h"
#include "Utils.h"
#include "FaceMySQL.h"
#include "ArmController.h"
#include "VirtualDataStoreZabbix.h"

int mainRoutine(int argc, char *argv[])
{
	g_type_init();
	asuraInit();
	MLPL_INFO("started asura: ver. %s\n", PACKAGE_VERSION);

	CommandLineArg cmdArg;
	for (int i = 1; i < argc; i++)
		cmdArg.push_back(argv[i]);

	FaceMySQL face(cmdArg);
	face.start();

	ArmController armController(cmdArg);
	armController.start();
	VirtualDataStoreZabbix *vdsZabbix
	  = VirtualDataStoreZabbix::getInstance();
	vdsZabbix->passCommandLineArg(cmdArg);

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;;
	try {
		ret = mainRoutine(argc, argv);
	} catch (const AsuraException &e){
		MLPL_ERR("Got exception: %s", e.getFancyMessage().c_str());
	} catch (const exception &e) {
		MLPL_ERR("Got exception: %s", e.what());
	}
	return ret;
}
