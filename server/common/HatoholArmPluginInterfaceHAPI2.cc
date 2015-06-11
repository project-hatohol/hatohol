/*
 * Copyright (C) 2015 Project Hatohol
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

#include "AMQPMessageHandler.h"
#include "AMQPConnectionInfo.h"
#include "AMQPConsumer.h"
#include "AMQPPublisher.h"
#include "HatoholArmPluginInterfaceHAPI2.h"
#include "JSONBuilder.h"

using namespace std;
using namespace mlpl;

std::list<HAPI2ProcedureDef> defaultValidProcedureList = {
	{PROCEDURE_BOTH,   "exchangeProfile",           SERVER_MANDATORY_HAP_OPTIONAL},
	{PROCEDURE_SERVER, "getMonitoringServerInfo",   SERVER_MANDATORY},
	{PROCEDURE_SERVER, "getLastInfo",               SERVER_MANDATORY},
	{PROCEDURE_SERVER, "putItems",                  SERVER_OPTIONAL},
	{PROCEDURE_SERVER, "putHistory",                SERVER_OPTIONAL},
	{PROCEDURE_SERVER, "updateHosts",               SERVER_OPTIONAL},
	{PROCEDURE_SERVER, "updateHostGroups",          SERVER_OPTIONAL},
	{PROCEDURE_SERVER, "updateHostGroupMembership", SERVER_OPTIONAL},
	{PROCEDURE_SERVER, "updateTriggers",            SERVER_OPTIONAL},
	{PROCEDURE_SERVER, "updateEvents",              SERVER_OPTIONAL},
	{PROCEDURE_SERVER, "updateHostParent",          SERVER_OPTIONAL},
	{PROCEDURE_SERVER, "updateArmInfo",             SERVER_MANDATORY},
	{PROCEDURE_HAP,    "fetchItems",                HAP_OPTIONAL},
	{PROCEDURE_HAP,    "fetchHistory",              HAP_OPTIONAL},
	{PROCEDURE_HAP,    "fetchTriggers",             HAP_OPTIONAL},
	{PROCEDURE_HAP,    "fetchEvents",               HAP_OPTIONAL},
};

struct JsonRpcObject {
	enum class Type {
		INVALID,
		PROCEDURE,
		NOTIFICATION,
		RESPONSE
	};

	JSONParser m_parser;
	Type m_type;
	string m_methodName;
	string m_id;
	string m_errorMessage;

	JsonRpcObject(const string &json)
	: m_parser(json), m_type(Type::INVALID)
	{
		parse(m_parser);
	}

	void parse(JSONParser &parser)
	{
		if (parser.hasError()) {
			const gchar *error = parser.getErrorMessage();
			m_errorMessage =
			  error ? error : "Failed to parse JSON";
			return;
		}

		if (parser.isMember("method")) {
			parseRequest(parser);
		} else {
			parseResponse(parser);
		}
	}

	bool parseId(JSONParser &parser, string &id)
	{
		JSONParser::ValueType type = parser.getValueType("id");
		switch(type) {
		case JSONParser::VALUE_TYPE_INT64:
		{
			int64_t value = 0;
			bool succeeded = parser.read("id", value);
			if (!succeeded)
				return false;
			id = StringUtils::sprintf("%" PRId64, value);
			return true;
		}
		case JSONParser::VALUE_TYPE_STRING:
			return parser.read("id", id);
		case JSONParser::VALUE_TYPE_NULL:
			return true;
		default:
			return false;
		}
	}

	void parseRequest(JSONParser &parser)
	{
		m_type = Type::INVALID;

		JSONParser::ValueType type = parser.getValueType("method");
		if (type != JSONParser::VALUE_TYPE_STRING) {
			m_errorMessage =
			  "Invalid request: "
			  "Invalid type for \"method\"!";
			return;
		}
		parser.read("method", m_methodName);

		if (!parser.isMember("id")) {
			m_type = Type::NOTIFICATION;
			return;
		}

		if (parser.isMember("id") && !parseId(parser, m_id)) {
			m_errorMessage = "Invalid request: Invalid id type!";
			return;
		}

		m_type = Type::PROCEDURE;
	}

	bool validateErrorObject(JSONParser &parser)
	{
		JSONParser::ValueType type = parser.getValueType("error");
		if (type != JSONParser::VALUE_TYPE_OBJECT) {
			m_errorMessage =
			  "Invalid type: \"error\" must be a object!";
			return false;
		}

		parser.startObject("error");
		type = parser.getValueType("code");
		if (type != JSONParser::VALUE_TYPE_INT64) {
			m_errorMessage =
			  "Invalid an error object: "
			  "\"code\" must be an integer!";
			parser.endObject();
			return false;
		}
		type = parser.getValueType("message");
		if (type != JSONParser::VALUE_TYPE_STRING) {
			m_errorMessage =
			  "Invalid an error object: "
			  "\"message\" must be a string!";
			parser.endObject();
			return false;
		}
		parser.endObject();
		return true;
	}

	void parseResponse(JSONParser &parser)
	{
		m_type = Type::INVALID;

		bool hasResult = parser.isMember("result");
		bool hasError = parser.isMember("error");

		if (!hasResult && !hasError) {
			m_errorMessage =
			  "Invalid JSON-RPC object: "
			  "None of method, result, error exist!";
			return;
		}

		if (hasResult && hasError) {
			m_errorMessage =
			  "Invalid request: "
			  "Must not exist both result and error!";
			return;
		}

		if (!parser.isMember("id")) {
			m_errorMessage =
			  "Invalid response: No id in the response!";
			return;
		}
		if (!parseId(parser, m_id)) {
			m_errorMessage = "Invalid response: Invalid id type!";
			return;
		}

		if (hasResult) {
			// The type of the result object will be determined by
			// the method.
		}

		if (hasError && !validateErrorObject(parser))
			return;

		m_type = Type::RESPONSE;
	}
};

class HatoholArmPluginInterfaceHAPI2::AMQPHAPI2MessageHandler
  : public AMQPMessageHandler
{
public:
	AMQPHAPI2MessageHandler(HatoholArmPluginInterfaceHAPI2 &hapi2)
	: m_hapi2(hapi2)
	{
	}

	enum class JsonRpcObjectType {
		INVALID,
		PROCEDURE,
		NOTIFICATION,
		RESPONSE
	};

	bool handle(AMQPConnection &connection, const AMQPMessage &message)
	{
		MLPL_DBG("message: <%s>/<%s>\n",
			 message.contentType.c_str(),
			 message.body.c_str());

		JsonRpcObject object(message.body);
		AMQPJSONMessage response;

		if (object.m_parser.hasError()) {
			response.body =
			  m_hapi2.buildErrorResponse(JSON_RPC_PARSE_ERROR,
						     "Invalid JSON",
						     NULL);
			MLPL_WARN("Invalid JSON: %s\n",
				  object.m_errorMessage.c_str());
			sendResponse(connection, response);
			return true;
		}

		switch(object.m_type) {
		case JsonRpcObject::Type::PROCEDURE:
			response.body = m_hapi2.interpretHandler(
					  object.m_methodName,
					  object.m_parser);
			sendResponse(connection, response);
			break;
		case JsonRpcObject::Type::NOTIFICATION:
			m_hapi2.interpretHandler(object.m_methodName,
						 object.m_parser);
			break;
		case JsonRpcObject::Type::RESPONSE:
			m_hapi2.handleResponse(object.m_id, object.m_parser);
			break;
		case JsonRpcObject::Type::INVALID:
		default:
			response.body =
			  m_hapi2.buildErrorResponse(JSON_RPC_INVALID_REQUEST,
						     object.m_errorMessage,
						     &object.m_parser);
			MLPL_WARN("Invalid JSON-RPC object: %s\n",
				  object.m_errorMessage.c_str());
			sendResponse(connection, response);
			break;
		}

		return true;
	}

	void sendResponse(AMQPConnection &connection,
			  const AMQPJSONMessage &response)
	{
		bool succeeded = connection.publish(response);
		if (!succeeded) {
			// TODO: retry?
		}
	}

private:
	HatoholArmPluginInterfaceHAPI2 &m_hapi2;
};

struct HatoholArmPluginInterfaceHAPI2::Impl
{
	CommunicationMode m_communicationMode;
	ArmPluginInfo m_pluginInfo;
	HatoholArmPluginInterfaceHAPI2 &m_hapi2;
	bool m_established;
	ProcedureHandlerMap m_procedureHandlerMap;
	map<string, ProcedureCallbackPtr> m_procedureCallbackMap;
	AMQPConnectionInfo m_connectionInfo;
	AMQPConsumer *m_consumer;
	AMQPHAPI2MessageHandler *m_handler;

	Impl(HatoholArmPluginInterfaceHAPI2 &hapi2,
	     const CommunicationMode mode)
	: m_communicationMode(mode),
	  m_hapi2(hapi2),
	  m_established(false),
	  m_consumer(NULL),
	  m_handler(NULL)
	{
	}

	~Impl()
	{
		if (m_consumer) {
			m_consumer->exitSync();
			delete m_consumer;
		}
		delete m_handler;
	}

	void setArmPluginInfo(const ArmPluginInfo &armPluginInfo)
	{
		m_pluginInfo = armPluginInfo;
	}

	void setupAMQPConnectionInfo(void)
	{
		AMQPConnectionInfo &info = m_connectionInfo;

		if (!m_pluginInfo.brokerUrl.empty())
			m_connectionInfo.setURL(m_pluginInfo.brokerUrl);
		string queueName;
		if (m_pluginInfo.staticQueueAddress.empty())
			queueName = generateQueueName(m_pluginInfo);
		else
			queueName = m_pluginInfo.staticQueueAddress;
		string queueNamePluginToServer = queueName + "-S";
		string queueNameServerToPlugin = queueName + "-T";

		if (m_communicationMode == MODE_SERVER) {
			info.setConsumerQueueName(queueNamePluginToServer);
			info.setPublisherQueueName(queueNameServerToPlugin);
		} else {
			info.setConsumerQueueName(queueNameServerToPlugin);
			info.setPublisherQueueName(queueNamePluginToServer);
		}
		info.setTLSCertificatePath(m_pluginInfo.tlsCertificatePath);
		info.setTLSKeyPath(m_pluginInfo.tlsKeyPath);
		info.setTLSCACertificatePath(m_pluginInfo.tlsCACertificatePath);
		info.setTLSVerifyEnabled(m_pluginInfo.isTLSVerifyEnabled());
	}

	void setupAMQPConnection(void)
	{
		setupAMQPConnectionInfo();

		m_handler = new AMQPHAPI2MessageHandler(m_hapi2);
		m_consumer = new AMQPConsumer(m_connectionInfo, m_handler);
	}

	string generateQueueName(const ArmPluginInfo &pluginInfo)
	{
		return StringUtils::sprintf("hapi2.%" FMT_SERVER_ID,
					    pluginInfo.serverId);
	}

	void start(void)
	{
		if (!m_consumer)
			setupAMQPConnection();
		if (!m_consumer) {
			MLPL_ERR("Failed to create AMQPConsumer!\n");
			return;
		}
		m_consumer->start();
	}

	void queueProcedureCallback(const string id,
				    ProcedureCallbackPtr callback)
	{
		m_procedureCallbackMap[id] = callback;
	}

	bool runProcedureCallback(const string id, JSONParser &parser)
	{
		auto it = m_procedureCallbackMap.find(id);
		if (it != m_procedureCallbackMap.end()) {
			ProcedureCallbackPtr callback = it->second;
			if (callback.hasData())
				callback->onGotResponse(parser);
			m_procedureCallbackMap.erase(it);
			return true;
		}
		return false;
	}
};

HatoholArmPluginInterfaceHAPI2::HatoholArmPluginInterfaceHAPI2(
  const CommunicationMode mode)
: m_impl(new Impl(*this, mode))
{
}

HatoholArmPluginInterfaceHAPI2::~HatoholArmPluginInterfaceHAPI2()
{
}

void HatoholArmPluginInterfaceHAPI2::setArmPluginInfo(
  const ArmPluginInfo &pluginInfo)
{
	m_impl->setArmPluginInfo(pluginInfo);
}

void HatoholArmPluginInterfaceHAPI2::registerProcedureHandler(
  const HAPI2ProcedureName &type, ProcedureHandler handler)
{
	m_impl->m_procedureHandlerMap[type] = handler;
}

string HatoholArmPluginInterfaceHAPI2::interpretHandler(
  const HAPI2ProcedureName &type, JSONParser &parser)
{
	if (!getEstablished() && type != HAPI2_EXCHANGE_PROFILE) {
		string message =
		  "Received a method while exchangeProfile isn't "
		  "completed yet!";
		// TODO: Determine the error code for it.
		return buildErrorResponse(JSON_RPC_SERVER_ERROR_BEGIN,
					  message, &parser);
	}

	auto it = m_impl->m_procedureHandlerMap.find(type);
	if (it == m_impl->m_procedureHandlerMap.end()) {
		string message = StringUtils::sprintf("Method not found: %s",
						      type.c_str());
		MLPL_WARN("An unknown method is called: %s\n", type.c_str());
		return buildErrorResponse(JSON_RPC_METHOD_NOT_FOUND,
					  message, &parser);
	}
	ProcedureHandler handler = it->second;
	return (this->*handler)(parser);
}

void HatoholArmPluginInterfaceHAPI2::handleResponse(
  const string id, JSONParser &parser)
{
	if (id.empty()) {
		string message;
		if (!parser.isMember("error")) {
			MLPL_WARN("Received an invalid message!");
			return;
		}
		parser.startObject("error");
		parser.read("message", message);
		MLPL_WARN("Received an error response: %s\n", message.c_str());
		parser.endObject();
		return;
	}

	bool found = m_impl->runProcedureCallback(id, parser);
	if (!found) {
		MLPL_WARN("Received an unknown response with id: %s\n",
			  id.c_str());
	}
}

void HatoholArmPluginInterfaceHAPI2::start(void)
{
	m_impl->start();
}

void HatoholArmPluginInterfaceHAPI2::send(const std::string &message)
{
	// TODO: Should use only one conection per one thread
	AMQPPublisher publisher(m_impl->m_connectionInfo);
	AMQPJSONMessage amqpMessage;
	amqpMessage.body = message;
	publisher.setMessage(amqpMessage);
	publisher.publish();
}

void HatoholArmPluginInterfaceHAPI2::send(
  const std::string &message, const int64_t id, ProcedureCallbackPtr callback)
{
	string idString = StringUtils::sprintf("%" PRId64, id);
	m_impl->queueProcedureCallback(idString, callback);
	send(message);
}

bool HatoholArmPluginInterfaceHAPI2::getEstablished(void)
{
	return m_impl->m_established;
}

void HatoholArmPluginInterfaceHAPI2::setEstablished(bool established)
{
	m_impl->m_established = established;
}

mt19937 HatoholArmPluginInterfaceHAPI2::getRandomEngine(void)
{
	std::random_device rd;
	std::mt19937 engine(rd());
	return engine;
}

bool HatoholArmPluginInterfaceHAPI2::setResponseId(
  JSONParser &requestParser, JSONBuilder &responseBuilder)
{
	bool succeeded = false;

	switch (requestParser.getValueType("id")) {
	case JSONParser::VALUE_TYPE_INT64:
	{
		int64_t numId;
		succeeded = requestParser.read("id", numId);
		if (succeeded)
			responseBuilder.add("id", numId);
		break;
	}
	case JSONParser::VALUE_TYPE_STRING:
	{
		string stringId;
		succeeded = requestParser.read("id", stringId);
		if (succeeded)
			responseBuilder.add("id", stringId);
		break;
	}
	default:
	{
		break;
	}
	}

	if (!succeeded) {
		// Should be checked before calling this function.
		MLPL_WARN("Cannot find valid id in the request!\n");
		responseBuilder.addNull("id");
	}

	return succeeded;
}

string HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
  const int errorCode, const string errorMessage, JSONParser *requestParser)
{
	JSONBuilder responseBuilder;
	responseBuilder.startObject();
	responseBuilder.add("jsonrpc", "2.0");
	if (requestParser)
		setResponseId(*requestParser, responseBuilder);
	else
		responseBuilder.addNull("id");
	responseBuilder.startObject("error");
	responseBuilder.add("code", errorCode);
	responseBuilder.add("message", errorMessage);
	responseBuilder.endObject(); // error
	responseBuilder.endObject();
	return responseBuilder.generate();
}

const std::list<HAPI2ProcedureDef> &
  HatoholArmPluginInterfaceHAPI2::getProcedureDefList(void) const
{
	return defaultValidProcedureList;
}
