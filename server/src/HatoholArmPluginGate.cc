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

#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include <string>
#include "HatoholArmPluginGate.h"
#include "CacheServiceDBClient.h"

using namespace std;
using namespace qpid::messaging;

struct HatoholArmPluginGate::PrivateContext
{
	MonitoringServerInfo serverInfo; // we have a copy.
	ArmStatus            armStatus;
	GPid                 pid;

	PrivateContext(const MonitoringServerInfo &_serverInfo)
	: serverInfo(_serverInfo),
	  pid(0)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::HatoholArmPluginGate(
  const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
}

bool HatoholArmPluginGate::start(const MonitoringSystemType &type)
{
	HatoholThreadBase::start();
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	ArmPluginInfo armPluginInfo;
	if (!dbConfig->getArmPluginInfo(armPluginInfo, type)) {
		MLPL_ERR("Failed to get ArmPluginInfo: type: %d\n", type);
		return false;
	}
	if (armPluginInfo.path.empty()) {
		MLPL_INFO("Started: passive plugin (%d) %s\n",
		          armPluginInfo.type, armPluginInfo.path.c_str());
		return true;
	}

	// launch a plugin process
	const gchar *workingDirectory = NULL;
	const gchar **envp = NULL;
	const gchar *argv[] = {armPluginInfo.path.c_str(), NULL};
	const GSpawnChildSetupFunc childSetup = NULL;
	const gpointer userData = NULL;
	GError *error = NULL;
	const GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD);
	gboolean succeeded =
	  g_spawn_async(workingDirectory, (gchar **)argv, (gchar **)envp,
	                flags, childSetup, userData, &m_ctx->pid, &error);
	if (!succeeded) {
		string reason = "<Unknown reason>";
		if (error) {
			reason = error->message;
			g_error_free(error);
		}
		MLPL_ERR("Failed to start: plugin (%d:%s), %s\n",
		         armPluginInfo.type, armPluginInfo.path.c_str(),
		         reason.c_str());
		return false;
	}

	// TODO: Setup to detect the terminate of the plugin.
	MLPL_INFO("Started: plugin (%d) %s\n",
	          armPluginInfo.type, armPluginInfo.path.c_str());
	m_ctx->armStatus.setRunningStatus(true);
	return true;
}

const ArmStatus &HatoholArmPluginGate::getArmStatus(void) const
{
	return m_ctx->armStatus;
}

gpointer HatoholArmPluginGate::mainThread(HatoholThreadArg *arg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);

	// The following lines is for checking if a build succeeds
	// and don't do a meaningful job.
	const string broker = "localhost:5672";
	const string address = "hapi";
	const string connectionOptions;

	Connection connection(broker, connectionOptions);
	connection.open();
	Session session = connection.createSession();

	Receiver receiver = session.createReceiver(address);
	Message message = receiver.fetch();
	session.acknowledge();
	connection.close();
	
	return NULL;
}

void HatoholArmPluginGate::waitExit(void)
{
	HatoholThreadBase::waitExit();
	m_ctx->armStatus.setRunningStatus(false);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::~HatoholArmPluginGate()
{
	if (m_ctx)
		delete m_ctx;
}

