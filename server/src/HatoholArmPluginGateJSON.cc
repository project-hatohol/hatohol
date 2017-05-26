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

#include <json-glib/json-glib.h>

#include <StringUtils.h>

#include "HatoholArmPluginGateJSON.h"
#include "ThreadLocalDBCache.h"
#include "UnifiedDataStore.h"
#include "ArmFake.h"
#include "AMQPConsumer.h"
#include "AMQPConnectionInfo.h"
#include "AMQPMessageHandler.h"
#include "GateJSONEventMessage.h"

using namespace std;
using namespace mlpl;

class AMQPJSONMessageHandler : public AMQPMessageHandler
{
public:
	AMQPJSONMessageHandler(const MonitoringServerInfo &serverInfo)
	: m_serverInfo(serverInfo),
	  m_hosts()
	{
		initializeHosts();
	}

	bool handle(AMQPConsumer &consumer, const AMQPMessage &message) override
	{
		// TODO: check content-type
		MLPL_DBG("message: <%s>/<%s>\n",
			 message.contentType.c_str(),
			 message.body.c_str());

		JsonParser *parser = json_parser_new();
		GError *error = NULL;
		if (json_parser_load_from_data(parser,
					       message.body.c_str(),
					       message.body.size(),
					       &error)) {
			process(json_parser_get_root(parser));
		} else {
			g_error_free(error);
		}
		g_object_unref(parser);
		return true;
	}

private:
	MonitoringServerInfo m_serverInfo;
	map<string, HostIdType> m_hosts;

	void initializeHosts()
	{
		UnifiedDataStore *uds = UnifiedDataStore::getInstance();
		HostsQueryOption option;
		option.setTargetServerId(m_serverInfo.id);

		ServerHostDefVect svHostDefVect;
		THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
		  uds->getServerHostDefs(svHostDefVect, option));

		ServerHostDefVectConstIterator it = svHostDefVect.begin();
		for (; it != svHostDefVect.end(); ++it) {
			const ServerHostDef &svHostDef = *it;
			m_hosts[svHostDef.name] = svHostDef.hostId;
		}
	}

	void process(JsonNode *root)
	{
		GateJSONEventMessage message(root);
		StringList errors;
		if (!message.validate(errors)) {
			StringListIterator it = errors.begin();
			for (; it != errors.end(); ++it) {
				string &errorMessage = *it;
				MLPL_ERR("%s\n", errorMessage.c_str());
			}
			return;
		}

		processEventMessage(message);
	}

	void processEventMessage(GateJSONEventMessage &message)
	{
		EventInfoList eventInfoList;
		EventInfo eventInfo;
		initEventInfo(eventInfo);
		eventInfo.serverId = m_serverInfo.id;
		eventInfo.id = message.getIDString();
		eventInfo.time = message.getTimestamp();
		eventInfo.type = message.getType();
		eventInfo.severity = message.getSeverity();
		eventInfo.hostName = message.getHostName();
		eventInfo.globalHostId = findOrCreateHostID(eventInfo.hostName);
		eventInfo.hostIdInServer = eventInfo.hostName;
		eventInfo.brief = message.getContent();
		eventInfoList.push_back(eventInfo);
		UnifiedDataStore::getInstance()->addEventList(eventInfoList);
	}

	HostIdType findOrCreateHostID(const string &hostName)
	{
		if (hostName.empty())
			return INVALID_HOST_ID;

		map<string, HostIdType>::iterator it;
		it = m_hosts.find(hostName);
		if (it != m_hosts.end()) {
			return it->second;
		}

		ServerHostDef svHostDef;
		svHostDef.id = AUTO_INCREMENT_VALUE;
		svHostDef.serverId = m_serverInfo.id;
		svHostDef.hostId = AUTO_ASSIGNED_ID;
		svHostDef.hostIdInServer = hostName;
		svHostDef.name = hostName;
		svHostDef.status = HOST_STAT_INAPPLICABLE;
		UnifiedDataStore *uds = UnifiedDataStore::getInstance();
		HostIdType hostId;
		THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
		  uds->upsertHost(svHostDef, &hostId));
		m_hosts[hostName] = hostId;
		return hostId;
	}
};

struct HatoholArmPluginGateJSON::Impl
{
	AMQPConnectionInfo m_connectionInfo;
	AMQPConsumer *m_consumer;
	AMQPJSONMessageHandler *m_handler;
	ArmFake m_armFake;
	ArmStatus m_armStatus;

	Impl(const MonitoringServerInfo &serverInfo)
	: m_connectionInfo(),
	  m_consumer(NULL),
	  m_handler(NULL),
	  m_armFake(serverInfo),
	  m_armStatus()
	{
		ThreadLocalDBCache cache;
		DBTablesConfig &dbConfig = cache.getConfig();
		const ServerIdType &serverId = serverInfo.id;
		ArmPluginInfo armPluginInfo;
		if (!dbConfig.getArmPluginInfo(armPluginInfo, serverId)) {
			MLPL_ERR("Failed to get ArmPluginInfo: serverId: %d\n",
				 serverId);
			return;
		}

		if (!armPluginInfo.brokerUrl.empty())
			m_connectionInfo.setURL(armPluginInfo.brokerUrl);

		string queueName;
		if (armPluginInfo.staticQueueAddress.empty())
			queueName = generateQueueName(serverInfo);
		else
			queueName = armPluginInfo.staticQueueAddress;
		m_connectionInfo.setConsumerQueueName(queueName);

		m_connectionInfo.setTLSCertificatePath(
			armPluginInfo.tlsCertificatePath);
		m_connectionInfo.setTLSKeyPath(
			armPluginInfo.tlsKeyPath);
		m_connectionInfo.setTLSCACertificatePath(
			armPluginInfo.tlsCACertificatePath);
		m_connectionInfo.setTLSVerifyEnabled(
			armPluginInfo.isTLSVerifyEnabled());

		m_handler = new AMQPJSONMessageHandler(serverInfo);
		m_consumer = new AMQPConsumer(m_connectionInfo, m_handler);
	}

	~Impl()
	{
		if (m_consumer) {
			m_consumer->exitSync();
			m_armStatus.setRunningStatus(false);
			delete m_consumer;
		}
		delete m_handler;
	}

	void start(void)
	{
		if (!m_consumer)
			return;
		m_consumer->start();
		m_armStatus.setRunningStatus(true);
	}

private:
	string generateQueueName(const MonitoringServerInfo &serverInfo)
	{
		return StringUtils::sprintf("gate.%" FMT_SERVER_ID,
					    serverInfo.id);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateJSON::HatoholArmPluginGateJSON(
  const MonitoringServerInfo &serverInfo,
  const bool &autoStart)
: m_impl(new Impl(serverInfo))
{
	if (autoStart) {
		m_impl->start();
	}
}

const MonitoringServerInfo
&HatoholArmPluginGateJSON::getMonitoringServerInfo(void) const
{
	return m_impl->m_armFake.getServerInfo();
}

const ArmStatus &HatoholArmPluginGateJSON::getArmStatus(void) const
{
	return m_impl->m_armStatus;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateJSON::~HatoholArmPluginGateJSON()
{
}
