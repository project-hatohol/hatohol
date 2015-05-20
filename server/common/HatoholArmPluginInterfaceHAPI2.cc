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

#include "AMQPConnectionInfo.h"
#include "HatoholArmPluginInterfaceHAPI2.h"

using namespace std;
using namespace mlpl;

struct HatoholArmPluginInterfaceHAPI2::Impl
{
	ArmPluginInfo m_pluginInfo;
	HatoholArmPluginInterfaceHAPI2 *hapi2;
	ProcedureHandlerMap procedureHandlerMap;
	AMQPConnectionInfo m_connectionInfo;

	Impl(HatoholArmPluginInterfaceHAPI2 *_hapi2)
	: hapi2(_hapi2)
	{
	}

	~Impl()
	{
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
	}

	string generateQueueName(const ArmPluginInfo &pluginInfo)
	{
		return StringUtils::sprintf("hapi2.%" FMT_SERVER_ID,
					    pluginInfo.serverId);
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

const AMQPConnectionInfo &
HatoholArmPluginInterfaceHAPI2::getAMQPConnectionInfo(void)
{
	return m_impl->m_connectionInfo;
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
