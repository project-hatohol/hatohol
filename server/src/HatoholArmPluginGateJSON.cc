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
	: m_serverInfo(serverInfo)
	{
	}

	bool handle(const amqp_envelope_t *envelope)
	{
		const amqp_bytes_t *content_type =
			&(envelope->message.properties.content_type);
		// TODO: check content-type
		const amqp_bytes_t *body = &(envelope->message.body);

		MLPL_DBG("message: <%.*s>/<%.*s>\n",
			 static_cast<int>(content_type->len),
			 static_cast<char *>(content_type->bytes),
			 static_cast<int>(body->len),
			 static_cast<char *>(body->bytes));

		JsonParser *parser = json_parser_new();
		GError *error = NULL;
		if (json_parser_load_from_data(parser,
					       static_cast<char *>(body->bytes),
					       static_cast<int>(body->len),
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
		eventInfo.id = message.getID();
		eventInfo.time = message.getTimestamp();
		eventInfo.type = EVENT_TYPE_BAD;
		eventInfo.severity = message.getSeverity();
		eventInfo.hostName = message.getHostName();
		eventInfo.brief = message.getContent();
		eventInfoList.push_back(eventInfo);
		UnifiedDataStore::getInstance()->addEventList(eventInfoList);
	}
};

struct HatoholArmPluginGateJSON::Impl
{
	AMQPConnectionInfo m_connectionInfo;
	AMQPConsumer *m_consumer;
	AMQPJSONMessageHandler *m_handler;
	ArmFake m_armFake;

	Impl(const MonitoringServerInfo &serverInfo)
	: m_connectionInfo(),
	  m_consumer(NULL),
	  m_handler(NULL),
	  m_armFake(serverInfo)
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
		m_connectionInfo.setQueueName(queueName);

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
			delete m_consumer;
		}
		delete m_handler;
	}

	void start(void)
	{
		if (!m_consumer)
			return;
		m_consumer->start();
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

ArmBase &HatoholArmPluginGateJSON::getArmBase(void)
{
	return m_impl->m_armFake;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateJSON::~HatoholArmPluginGateJSON()
{
}
