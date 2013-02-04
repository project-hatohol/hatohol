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

#ifndef ArmController_h
#define ArmController_h

#include <map>
using namespace std;

#include <SmartQueue.h>
using namespace mlpl;

#include "AsuraThreadBase.h"

class ArmController;
typedef bool (ArmController::*ArmFactory)(string &arg);

class ArmController : public AsuraThreadBase {
public:
	ArmController(CommandLineArg &cmdArg);

protected:
	bool factoryZabbix(string &arg);

	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);

private:
	SmartQueue m_cmdQueue;
	SmartQueue m_armQueue;
	map<string, ArmFactory> m_armFactoryMap;

	size_t parseCmdArgArm(CommandLineArg &cmdArg, size_t idx);
};

#endif // ArmController_h
