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

#include <cmath>
#include <json-glib/json-glib.h>

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

	bool validateObjectMember(const gchar *context,
				  JsonObject *object,
				  const gchar *name,
				  GType expectedValueType)
	{
		JsonNode *memberNode = json_object_get_member(object, name);
		if (!memberNode) {
			MLPL_ERR("%s.%s must exist\n", context, name);
			return false;
		}

		GType valueType = json_node_get_value_type(memberNode);
		if (valueType != expectedValueType) {
			MLPL_ERR("%s.%s must be %s: %s\n",
				 context,
				 name,
				 g_type_name(expectedValueType),
				 g_type_name(valueType));
			return false;
		}

		return true;
	}

	bool validateMessage(JsonNode *root)
	{
		if (json_node_get_node_type(root) != JSON_NODE_OBJECT) {
			MLPL_ERR("JSON message must be an object");
			return false;
		}

		JsonObject *rootObject = json_node_get_object(root);
		if (!validateObjectMember("{root}", rootObject, "type",
					  G_TYPE_STRING))
			return false;
		if (!validateObjectMember("{root}", rootObject, "body",
					  JSON_TYPE_OBJECT))
			return false;

		return true;
	}

	void process(JsonNode *root)
	{
		if (!validateMessage(root))
			return;

		JsonObject *rootObject = json_node_get_object(root);
		const gchar *type =
			json_object_get_string_member(rootObject, "type");
		if (string(type) != "event") {
			MLPL_ERR("{root}.type must be \"event\" for now\n");
			return;
		}

		JsonNode *bodyNode = json_object_get_member(rootObject, "body");
		JsonObject *body = json_node_get_object(bodyNode);
		processEventMessage(body);
	}

	bool validateBodyMember(JsonObject *body,
				const gchar *name,
				GType expectedValueType)
	{
		return validateObjectMember("body", body, name,
					    expectedValueType);
	}

	bool validateEventBody(JsonObject *body)
	{
		if (!validateBodyMember(body, "id", G_TYPE_INT64))
			return false;
		if (!validateBodyMember(body, "timestamp", G_TYPE_DOUBLE))
			return false;
		if (!validateBodyMember(body, "hostName", G_TYPE_STRING))
			return false;
		if (!validateBodyMember(body, "message", G_TYPE_STRING))
			return false;
		return true;
	}

	long secondToNanoSecond(gdouble second)
	{
		return second * 1000000000;
	}

	void processEventMessage(JsonObject *body)
	{
		if (!validateEventBody(body))
		    return;

		EventInfoList eventInfoList;
		EventInfo eventInfo;
		initEventInfo(eventInfo);
		eventInfo.serverId = m_serverInfo.id;
		eventInfo.id = json_object_get_int_member(body, "id");
		gdouble timestamp =
			json_object_get_double_member(body, "timestamp");
		eventInfo.time.tv_sec = static_cast<time_t>(timestamp);
		eventInfo.time.tv_nsec =
			secondToNanoSecond(fmod(timestamp, 1.0));
		eventInfo.type = EVENT_TYPE_BAD;
		eventInfo.hostName =
			json_object_get_string_member(body, "hostName");
		eventInfo.brief =
			json_object_get_string_member(body, "message");
		eventInfoList.push_back(eventInfo);
		UnifiedDataStore::getInstance()->addEventList(eventInfoList);
	}
};

struct HatoholArmPluginGateJSON::Impl
{
	AMQPConsumer *m_consumer;
	AMQPJSONMessageHandler *m_handler;
	ArmFake m_armFake;

	Impl(const MonitoringServerInfo &serverInfo)
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

		m_handler = new AMQPJSONMessageHandler(serverInfo);
		m_consumer = new AMQPConsumer(brokerUrl,
					      queueAddress,
					      m_handler);
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
