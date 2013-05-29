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

#include "ArmNagiosNDOUtils.h"
#include "DBAgentMySQL.h"

using namespace std;

struct ArmNagiosNDOUtils::PrivateContext
{
	DBAgentMySQL dbAgent;

	// methods
	PrivateContext(const MonitoringServerInfo &serverInfo)
	: dbAgent(serverInfo.dbName.c_str(), serverInfo.userName.c_str(),
	          serverInfo.password.c_str(),
	          serverInfo.getHostAddress(), serverInfo.port)
	{
	}

	virtual ~PrivateContext()
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmNagiosNDOUtils::ArmNagiosNDOUtils(const MonitoringServerInfo &serverInfo)
: ArmBase(serverInfo),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
}

ArmNagiosNDOUtils::~ArmNagiosNDOUtils()
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	if (m_ctx)
		delete m_ctx;
	MLPL_INFO("ArmNagiosNDOUtils [%d:%s]: exit process completed.\n",
	          svInfo.id, svInfo.hostName.c_str());
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer ArmNagiosNDOUtils::mainThread(AsuraThreadArg *arg)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	MLPL_INFO("started: ArmNagiosNDOUtils (server: %s)\n",
	          svInfo.hostName.c_str());
	return ArmBase::mainThread(arg);
}

bool ArmNagiosNDOUtils::mainThreadOneProc(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}


