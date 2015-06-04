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
#include "HAPI2Procedure.h"
#include "HatoholArmPluginInterfaceHAPI2.h"
#include "JSONBuilder.h"

using namespace std;
using namespace mlpl;

class HatoholArmPluginInterfaceHAPI2::AMQPHAPI2MessageHandler
  : public AMQPMessageHandler
{
public:
	AMQPHAPI2MessageHandler(HatoholArmPluginInterfaceHAPI2 &hapi2)
	: m_hapi2(hapi2)
	{
	}

	bool handle(AMQPConnection &connection, const AMQPMessage &message)
	{
		MLPL_DBG("message: <%s>/<%s>\n",
			 message.contentType.c_str(),
			 message.body.c_str());

		JsonParser *parser = json_parser_new();
		GError *error = NULL;
		if (json_parser_load_from_data(parser,
					       message.body.c_str(),
					       message.body.size(),
					       &error)) {
			process(connection, json_parser_get_root(parser), message.body);
		} else {
			g_error_free(error);
		}
		g_object_unref(parser);
		return true;
	}

private:
	void process(AMQPConnection &connection, JsonNode *root, const string &body)
	{
		HAPI2Procedure procedure(root);
		StringList errors;
		HAPI2ProcedureType type = HAPI2_PROCEDURE_TYPE_BAD;
		AMQPJSONMessage message;
		if (procedure.validate(errors)) {
			type = procedure.getType();
			message.body = m_hapi2.interpretHandler(type, body);
		} else {
			for (auto errorMessage : errors)
				MLPL_ERR("%s\n", errorMessage.c_str());
			string error = StringUtils::sprintf("Method not found");
			JSONParser parser(body);
			message.body =
			  m_hapi2.buildErrorResponse(JSON_RPC_METHOD_NOT_FOUND,
						     error, &parser);
		}
		bool succeeded = connection.publish(message);
		if (!succeeded) {
			// TODO: retry?
		}
		m_hapi2.onHandledCommand(type);
	}

private:
	HatoholArmPluginInterfaceHAPI2 &m_hapi2;
};

struct HatoholArmPluginInterfaceHAPI2::Impl
{
	CommunicationMode m_communicationMode;
	ArmPluginInfo m_pluginInfo;
	HatoholArmPluginInterfaceHAPI2 &hapi2;
	ProcedureHandlerMap procedureHandlerMap;
	AMQPConnectionInfo m_connectionInfo;
	AMQPConsumer *m_consumer;
	AMQPHAPI2MessageHandler *m_handler;

	Impl(HatoholArmPluginInterfaceHAPI2 &_hapi2, const CommunicationMode mode)
	: m_communicationMode(mode),
	  hapi2(_hapi2),
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

		m_handler = new AMQPHAPI2MessageHandler(hapi2);
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
  const HAPI2ProcedureType &type, ProcedureHandler handler)
{
	m_impl->procedureHandlerMap[type] = handler;
}

string HatoholArmPluginInterfaceHAPI2::interpretHandler(
  const HAPI2ProcedureType &type, const string json)
{
	ProcedureHandlerMapConstIterator it =
	  m_impl->procedureHandlerMap.find(type);
	if (it == m_impl->procedureHandlerMap.end()) {
		// TODO: Add a supplied method name
		string message = StringUtils::sprintf("Method not found");
		return buildErrorResponse(JSON_RPC_METHOD_NOT_FOUND,
					  message);
	}
	ProcedureHandler handler = it->second;
	return (this->*handler)(json);
}

void HatoholArmPluginInterfaceHAPI2::onHandledCommand(const HAPI2ProcedureType &type)
{
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
		// TODO: Should return an error response
		MLPL_WARN("Cannot find valid id in the request!");
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
