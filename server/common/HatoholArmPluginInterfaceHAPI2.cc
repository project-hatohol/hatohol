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
#include "GateJSONProcedureHAPI2.h"
#include "HatoholArmPluginInterfaceHAPI2.h"

using namespace std;
using namespace mlpl;

class AMQPHAPI2MessageHandler
: public AMQPMessageHandler, public HatoholArmPluginInterfaceHAPI2
{
public:
	AMQPHAPI2MessageHandler()
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
			process(json_parser_get_root(parser));
		} else {
			g_error_free(error);
		}
		g_object_unref(parser);
		return true;
	}

private:
	void process(JsonNode *root)
	{
		GateJSONProcedureHAPI2 procedure(root);
		StringList errors;
		if (!procedure.validate(errors)) {
			for (auto errorMessage : errors) {
				MLPL_ERR("%s\n", errorMessage.c_str());
			}
			return;
		}
		string params = procedure.getParams();
		interpretHandler(procedure.getProcedureType(), params, root);
	}
};

struct HatoholArmPluginInterfaceHAPI2::Impl
{
	ArmPluginInfo m_pluginInfo;
	HatoholArmPluginInterfaceHAPI2 *hapi2;
	ProcedureHandlerMap procedureHandlerMap;
	AMQPConnectionInfo m_connectionInfo;
	AMQPConsumer *m_consumer;
	AMQPHAPI2MessageHandler *m_handler;

	Impl(HatoholArmPluginInterfaceHAPI2 *_hapi2)
	: hapi2(_hapi2),
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
		setupAMQPConnection();
	}

	void setupAMQPConnectionInfo(void)
	{
		if (!m_pluginInfo.brokerUrl.empty())
			m_connectionInfo.setURL(m_pluginInfo.brokerUrl);
		string queueName;
		if (m_pluginInfo.staticQueueAddress.empty())
			queueName = generateQueueName(m_pluginInfo);
		else
			queueName = m_pluginInfo.staticQueueAddress;
		m_connectionInfo.setQueueName(queueName);

		m_connectionInfo.setTLSCertificatePath(
			m_pluginInfo.tlsCertificatePath);
		m_connectionInfo.setTLSKeyPath(
			m_pluginInfo.tlsKeyPath);
		m_connectionInfo.setTLSCACertificatePath(
			m_pluginInfo.tlsCACertificatePath);
		m_connectionInfo.setTLSVerifyEnabled(
			m_pluginInfo.isTLSVerifyEnabled());
	}

	void setupAMQPConnection(void)
	{
		setupAMQPConnectionInfo();

		m_handler = new AMQPHAPI2MessageHandler();
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
			return;
		m_consumer->start();
	}
};

HatoholArmPluginInterfaceHAPI2::HatoholArmPluginInterfaceHAPI2()
: m_impl(new Impl(this))
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

void HatoholArmPluginInterfaceHAPI2::interpretHandler(
  const HAPI2ProcedureType &type, const string params, JsonNode *root)
{
	ProcedureHandlerMapConstIterator it =
	  m_impl->procedureHandlerMap.find(type);
	if (it == m_impl->procedureHandlerMap.end()) {
		// TODO: reply error
		return;
	}
	ProcedureHandler handler = it->second;
	string replyJSON = (this->*handler)(&type, params);
	// TODO: send replyJSON
	onHandledCommand(type);
}

void HatoholArmPluginInterfaceHAPI2::onHandledCommand(const HAPI2ProcedureType &type)
{
}

void HatoholArmPluginInterfaceHAPI2::start(void)
{
	m_impl->start();
}
