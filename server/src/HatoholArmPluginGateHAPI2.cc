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

#include <json-glib/json-glib.h>

#include <StringUtils.h>

#include "HatoholArmPluginInterfaceHAPI2.h"
#include "HatoholArmPluginGateHAPI2.h"
#include "ThreadLocalDBCache.h"
#include "UnifiedDataStore.h"
#include "ArmFake.h"
#include "AMQPConsumer.h"
#include "AMQPConnectionInfo.h"
#include "AMQPMessageHandler.h"
#include "GateJSONProcedureHAPI2.h"

using namespace std;
using namespace mlpl;

HAPI2ProcedureInfoList defaultValidProcedureList[] = {
	{PROCEDURE_BOTH,   "exchangeProfile",           BOTH_MANDATORY},
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

class AMQPHAPI2MessageHandler
: public AMQPMessageHandler, public HatoholArmPluginInterfaceHAPI2
{
public:
	AMQPHAPI2MessageHandler(const MonitoringServerInfo &serverInfo)
	: m_serverInfo(serverInfo),
	  m_hosts()
	{
		initializeHosts();
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
		GateJSONProcedureHAPI2 hapi2(root);
		StringList errors;
		if (!hapi2.validate(errors)) {
			StringListIterator it = errors.begin();
			for (; it != errors.end(); ++it) {
				string &errorMessage = *it;
				MLPL_ERR("%s\n", errorMessage.c_str());
			}
			return;
		}
		HAPI2ProcedureType hapi2Type = hapi2.getProcedureType();
		switch(hapi2Type) {
		case HAPI2_EXCHANGE_PROFILE:
		case HAPI2_MONITORING_SERVER_INFO:
		case HAPI2_LAST_INFO:
		case HAPI2_PUT_ITEMS:
		case HAPI2_PUT_HISTORY:
		case HAPI2_UPDATE_HOSTS:
		case HAPI2_UPDATE_HOST_GROUPS:
		case HAPI2_UPDATE_HOST_GROUP_MEMEBRSHIP:
		case HAPI2_UPDATE_TRIGGERS:
		case HAPI2_UPDATE_EVENTS:
		case HAPI2_UPDATE_HOST_PARENT:
		case HAPI2_UPDATE_ARM_INFO:
			interpretHandler(hapi2Type, root);
			break;
		default:
			// TODO: reply error
			break;
		}
	}
};

struct HatoholArmPluginGateHAPI2::Impl
{
	// We have a copy. The access to the object is MT-safe.
	const MonitoringServerInfo m_serverInfo;
	AMQPConnectionInfo m_connectionInfo;
	AMQPConsumer *m_consumer;
	AMQPHAPI2MessageHandler *m_handler;
	ArmFake m_armFake;
	ArmStatus m_armStatus;

	Impl(const MonitoringServerInfo &_serverInfo,
	     HatoholArmPluginGateHAPI2 *hapghapi)
	: m_serverInfo(_serverInfo),
	  m_connectionInfo(),
	  m_consumer(NULL),
	  m_handler(NULL),
	  m_armFake(m_serverInfo),
	  m_armStatus()
	{
		ThreadLocalDBCache cache;
		DBTablesConfig &dbConfig = cache.getConfig();
		const ServerIdType &serverId = m_serverInfo.id;
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
			queueName = generateQueueName(m_serverInfo);
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

		m_handler = new AMQPHAPI2MessageHandler(m_serverInfo);
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
		return StringUtils::sprintf("hapi2.%" FMT_SERVER_ID,
					    serverInfo.id);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateHAPI2::HatoholArmPluginGateHAPI2(
  const MonitoringServerInfo &serverInfo)
: m_impl(new Impl(serverInfo, this))
{
	registerProcedureHandler(
	  HAPI2_EXCHANGE_PROFILE,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerExchangeProfile);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateHAPI2::~HatoholArmPluginGateHAPI2()
{
}

const MonitoringServerInfo &
HatoholArmPluginGateHAPI2::getMonitoringServerInfo(void) const
{
	return m_impl->m_armFake.getServerInfo();
}

const ArmStatus &HatoholArmPluginGateHAPI2::getArmStatus(void) const
{
	return m_impl->m_armStatus;
}

void HatoholArmPluginGateHAPI2::procedureHandlerExchangeProfile(
  const HAPI2ProcedureType *type)
{
	// TODO: implement exchange profile procedure
}
