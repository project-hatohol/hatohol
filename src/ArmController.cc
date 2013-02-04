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

#include <Logger.h>
#include <SmartQueue.h>
#include <StringUtils.h>
using namespace mlpl;

#include "ArmController.h"
#include "ArmZabbixAPI.h"

class armQueueElement : public SmartQueueElement {
public:
	armQueueElement(ArmBase *arm)
	: m_arm(arm)
	{
	}

private:
	ArmBase *m_arm;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmController::ArmController(CommandLineArg &cmdArg)
{
	m_armFactoryMap["zabbix"] = &ArmController::factoryZabbix;

	for (size_t i = 0; i < cmdArg.size(); i++) {
		if (cmdArg[i] == "--arm") {
			i = parseCmdArgArm(cmdArg, i);
		}
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer ArmController::mainThread(AsuraThreadArg *arg)
{
	MLPL_INFO("started: %s\n", __PRETTY_FUNCTION__);
	while (true) {
		m_cmdQueue.pop();
	}
	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
size_t ArmController::parseCmdArgArm(CommandLineArg &cmdArg, size_t idx)
{
	if (idx == cmdArg.size() - 1) {
		MLPL_ERR("needs Arm definition.");
		return idx;
	}

	idx++;
	vector<string> armDef;
	StringUtils::split(armDef, cmdArg[idx], ':');
	if (armDef.size() != 2) {
		MLPL_ERR("invalid definition: %s\n", cmdArg[idx].c_str());
		return idx;
	}
	string &name = armDef[0];
	string &arg = armDef[1];

	// Search the factory
	map<string, ArmFactory>::iterator it = m_armFactoryMap.find(name);
	if (it == m_armFactoryMap.end()) {
		MLPL_ERR("Not found factory for arm name: %s, %s\n",
		         name.c_str(), cmdArg[idx].c_str());
		return idx;
	}

	// Arm production
	ArmFactory &factory = it->second;
	if (!(this->*factory)(arg)) {
		MLPL_ERR("Failed to create Arm: %s\n", cmdArg[idx].c_str());
		return idx;
	}

	return idx;
}

bool ArmController::factoryZabbix(string &arg)
{
	ArmBase *arm = new ArmZabbixAPI(arg.c_str());
	arm->start(true);
	return true;
}
