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

#include "HatoholArmPluginGateJSON.h"
#include "CacheServiceDBClient.h"
#include "UnifiedDataStore.h"
#include "ArmFake.h"
#include "AMQPConsumer.h"
#include "AMQPMessageHandler.h"

using namespace std;
using namespace mlpl;

static const string DEFAULT_BROKER_URL = "localhost:5672";

class AMQPJSONMessageHandler : public AMQPMessageHandler
{
public:
	bool handle(const amqp_envelope_t *envelope)
	{
		const amqp_bytes_t *content_type =
			&(envelope->message.properties.content_type);
		const amqp_bytes_t *body = &(envelope->message.body);
		MLPL_INFO("message: <%.*s>/<%.*s>\n",
			  static_cast<int>(content_type->len),
			  static_cast<char *>(content_type->bytes),
			  static_cast<int>(body->len),
			  static_cast<char *>(body->bytes));
		return true;
	};
};

struct HatoholArmPluginGateJSON::PrivateContext
{
	AMQPConsumer *m_consumer;
	AMQPJSONMessageHandler *m_handler;
	ArmFake m_armFake;

	PrivateContext(const MonitoringServerInfo &serverInfo)
	: m_consumer(NULL),
	  m_handler(NULL),
	  m_armFake(serverInfo)
	{
		CacheServiceDBClient cache;
		DBClientConfig *dbConfig = cache.getConfig();
		const ServerIdType &serverId = serverInfo.id;
		ArmPluginInfo armPluginInfo;
		if (!dbConfig->getArmPluginInfo(armPluginInfo, serverId)) {
			MLPL_ERR("Failed to get ArmPluginInfo: serverId: %d\n",
				 serverId);
			return;
		}

		string brokerUrl(armPluginInfo.brokerUrl);
		if (brokerUrl.empty())
			brokerUrl = DEFAULT_BROKER_URL;

		string queueAddress;
		if (armPluginInfo.staticQueueAddress.empty())
			queueAddress = generateBrokerAddress(serverInfo);
		else
			queueAddress = armPluginInfo.staticQueueAddress;

		m_handler = new AMQPJSONMessageHandler();
		m_consumer = new AMQPConsumer(brokerUrl,
					      queueAddress,
					      m_handler);
	}

	~PrivateContext()
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
	string generateBrokerAddress(const MonitoringServerInfo &serverInfo)
	{
		return StringUtils::sprintf("hatohol.gate.%" FMT_SERVER_ID,
					    serverInfo.id);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateJSON::HatoholArmPluginGateJSON(
  const MonitoringServerInfo &serverInfo,
  const bool &autoStart)
: m_ctx(new PrivateContext(serverInfo))
{
	if (autoStart) {
		m_ctx->start();
	}
}

ArmBase &HatoholArmPluginGateJSON::getArmBase(void)
{
	return m_ctx->m_armFake;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateJSON::~HatoholArmPluginGateJSON()
{
	delete m_ctx;
}
