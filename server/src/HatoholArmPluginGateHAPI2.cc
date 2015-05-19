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
#include <JSONParser.h>

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

		for (auto svHostDef : svHostDefVect) {
			m_hosts[svHostDef.name] = svHostDef.hostId;
		}
	}

	void process(JsonNode *root)
	{
		GateJSONProcedureHAPI2 hapi2(root);
		StringList errors;
		if (!hapi2.validate(errors)) {
			for (auto errorMessage : errors) {
				MLPL_ERR("%s\n", errorMessage.c_str());
			}
			return;
		}
		string params = hapi2.getParams();
		interpretHandler(hapi2.getProcedureType(), params, root);
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
	registerProcedureHandler(
	  HAPI2_MONITORING_SERVER_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerMonitoringServerInfo);
	registerProcedureHandler(
	  HAPI2_LAST_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerLastInfo);
	registerProcedureHandler(
	  HAPI2_PUT_ITEMS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutItems);
	registerProcedureHandler(
	  HAPI2_PUT_HISTORY,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHistory);
	registerProcedureHandler(
	  HAPI2_UPDATE_HOSTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateHosts);
	registerProcedureHandler(
	  HAPI2_UPDATE_EVENTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateEvents);
}

bool HatoholArmPluginGateHAPI2::parseTimeStamp(
  const string &timeStampString, timespec &timeStamp)
{
	timeStamp.tv_sec = 0;
	timeStamp.tv_nsec = 0;

	StringVector list;
	StringUtils::split(list, timeStampString, '.', false);

	if (list.empty() || list.size() > 2)
		goto ERR;
	struct tm tm;
	if (!strptime(list[0].c_str(), "%4Y%2m%2d%2H%2M%2S", &tm))
		goto ERR;
	timeStamp.tv_sec = timegm(&tm); // as UTC

	if (list.size() == 1)
		return true;

	if (list[1].size() > 9)
		goto ERR;
	for (size_t i = 0; i < list[1].size(); i++) {
		unsigned int ch = list[1][i];
		if (ch < '0' || ch > '9')
			goto ERR;
	}
	for (size_t i = list[1].size(); i < 9; i++)
		list[1] += '0';
	timeStamp.tv_nsec = atol(list[1].c_str());

	return true;

 ERR:
	MLPL_ERR("Invalid timestamp format: %s\n",
		 timeStampString.c_str());
	return false;
}

static bool parseTimeStamp(
  JSONParser &parser, const string &member, timespec &timeStamp)
{
	string timeStampString;
	parser.read(member, timeStampString);
	return HatoholArmPluginGateHAPI2::parseTimeStamp(timeStampString,
							 timeStamp);
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

string HatoholArmPluginGateHAPI2::procedureHandlerExchangeProfile(
  const HAPI2ProcedureType type, const string &params)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.startObject("result");
	agent.startArray("procedures");
	for (auto defaultValidProcedureDef : defaultValidProcedureList) {
		if (defaultValidProcedureDef.type == PROCEDURE_BOTH ||
		    defaultValidProcedureDef.type == PROCEDURE_HAP)
			continue;
		agent.add(defaultValidProcedureDef.name);
	}
	agent.endArray(); // procedures
	agent.endObject(); // result
	agent.add("name", "exampleName"); // TODO: add process name mechanism
	agent.add("id", 1);
	agent.endObject();
	// TODO: implement replying exchange profile procedure with AMQP
	return agent.generate();
}

string HatoholArmPluginGateHAPI2::procedureHandlerMonitoringServerInfo(
  const HAPI2ProcedureType type, const string &params)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.startObject("result");
	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	agent.add("serverId", serverInfo.id);
	agent.add("url", serverInfo.baseURL);
	agent.add("type", serverInfo.type);
	agent.add("nickName", serverInfo.nickname);
	agent.add("userName", serverInfo.userName);
	agent.add("password", serverInfo.password);
	agent.add("pollingIntervalSec", serverInfo.pollingIntervalSec);
	agent.add("retryIntervalSec", serverInfo.retryIntervalSec);
	// TODO: Use serverInfo.extendedInfo
	agent.add("extendedInfo", "exampleExtraInfo");
	agent.endObject(); // result
	agent.add("id", 1);
	agent.endObject();
	// TODO: implement replying exchange profile procedure with AMQP
	return agent.generate();
}

string HatoholArmPluginGateHAPI2::procedureHandlerLastInfo(
  const HAPI2ProcedureType type, const string &params)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList triggerInfoList;
	TriggersQueryOption triggersQueryOption(USER_ID_SYSTEM);
	dataStore->getTriggerList(triggerInfoList, triggersQueryOption);
	TriggerInfoListIterator it = triggerInfoList.begin();
	TriggerInfo &firstTriggerInfo = *it;

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", firstTriggerInfo.lastChangeTime.tv_sec);
	agent.add("id", 1);
	agent.endObject();
	return agent.generate();
}

static bool parseItemParams(JSONParser &parser, ItemInfoList &itemInfoList)
{
	parser.startObject("params");
	parser.startObject("items");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse item contents.\n");
			return false;
		}

		ItemInfo itemInfo;
		parser.read("itemId", itemInfo.id);
		parser.read("hostId", itemInfo.hostIdInServer);
		parser.read("brief", itemInfo.brief);
		parseTimeStamp(parser, "lastValueTime", itemInfo.lastValueTime);
		parser.read("lastValue", itemInfo.lastValue);
		parser.read("itemGroupName", itemInfo.itemGroupName);
		parser.read("unit", itemInfo.unit);
		parser.endElement();

		itemInfoList.push_back(itemInfo);
	}
	parser.endObject(); // items
	parser.endObject(); // params
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutItems(
  const HAPI2ProcedureType type, const string &params)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ItemInfoList itemList;
	JSONParser parser(params);
	bool succeeded = parseItemParams(parser, itemList);
	dataStore->addItemList(itemList);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", "");
	agent.add("id", 1);
	agent.endObject();
	// TODO: implement replying exchange profile procedure with AMQP
	return agent.generate();
}

static bool parseHistoryParams(JSONParser &parser, HistoryInfoVect &historyInfoVect)
{
	parser.startObject("params");
	ItemIdType itemId = "";
	parser.read("itemId", itemId);
	parser.startObject("histories");
	size_t num = parser.countElements();

	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse histories contents.\n");
			return false;
		}

		HistoryInfo historyInfo;
		historyInfo.itemId = itemId;
		parser.read("value", historyInfo.value);
		parseTimeStamp(parser, "time", historyInfo.clock);
		parser.endElement();

		historyInfoVect.push_back(historyInfo);
	}
	parser.endObject(); // histories
	parser.endObject(); // params
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHistory(
  const HAPI2ProcedureType type, const string &params)
{
	HistoryInfoVect historyInfoVect;
	JSONParser parser(params);
	bool succeeded = parseHistoryParams(parser, historyInfoVect);
	// TODO: Store or transport historyInfoVect

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", "");
	agent.add("id", 1);
	agent.endObject();
	// TODO: implement replying exchange profile procedure with AMQP
	return agent.generate();
}

static bool parseHostsParams(JSONParser &parser, ServerHostDefVect &hostInfoVect)
{
	parser.startObject("params");
	parser.startObject("hosts");
	size_t num = parser.countElements();
	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			return false;
		}

		ServerHostDef hostInfo;
		int64_t hostId;
		parser.read("hostId", hostId);
		hostInfo.hostIdInServer = hostId;
		parser.read("hostName", hostInfo.name);
		parser.endElement();

		hostInfoVect.push_back(hostInfo);
	}
	parser.endObject(); // hosts
	parser.endObject(); // params
	return true;
};

static bool parseHostsUpdateType(JSONParser &parser, string &updateType)
{
	parser.read("updateType", updateType);
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateHosts(
  const HAPI2ProcedureType type, const string &params)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerHostDefVect hostInfoVect;
	JSONParser parser(params);
	bool succeeded = parseHostsParams(parser, hostInfoVect);
	string updateType;
	bool checkInvalidHosts = parseHostsUpdateType(parser, updateType);
	// TODO: implement validation for Hosts
	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		// TODO: implement storing LastInfo data
	}

	dataStore->upsertHosts(hostInfoVect);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", result);
	agent.add("id", 1);
	agent.endObject();
	// TODO: implement replying exchange profile procedure with AMQP
	return agent.generate();
}

static bool parseEventsParams(JSONParser &parser, EventInfoList &eventInfoList)
{
	parser.startObject("params");
	parser.startObject("events");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse event contents.\n");
			return false;
		}

		EventInfo eventInfo;
		parser.read("eventId",      eventInfo.id);
		parseTimeStamp(parser, "time", eventInfo.time);
		int64_t type, status, severity;
		parser.read("type",         type);
		eventInfo.type = (EventType)type;
		parser.read("triggerId",    eventInfo.triggerId);
		parser.read("status",       status);
		eventInfo.status = (TriggerStatusType)status;
		parser.read("severity",     severity);
		eventInfo.severity = (TriggerSeverityType)severity;
		parser.read("hostId",       eventInfo.hostIdInServer);
		parser.read("hostName",     eventInfo.hostName);
		parser.read("brief",        eventInfo.brief);
		parser.read("extendedInfo", eventInfo.extendedInfo);
		parser.endElement();

		eventInfoList.push_back(eventInfo);
	}
	parser.endObject(); // events
	parser.endObject(); // params
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateEvents(
  const HAPI2ProcedureType type, const string &params)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList eventInfoList;
	JSONParser parser(params);
	bool succeeded = parseEventsParams(parser, eventInfoList);
	string result = succeeded ? "SUCCESS" : "FAILURE";
	dataStore->addEventList(eventInfoList);
	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_EVENT);
	}

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", result);
	agent.add("id", 1);
	agent.endObject();
	// TODO: implement replying exchange profile procedure with AMQP
	return agent.generate();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

void HatoholArmPluginGateHAPI2::upsertLastInfo(string lastInfoValue, LastInfoType type)
{
	ThreadLocalDBCache cache;
	DBTablesLastInfo &dbLastInfo = cache.getLastInfo();
	OperationPrivilege privilege(USER_ID_SYSTEM);
	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	LastInfoDef lastInfo;
	lastInfo.dataType = type;
	lastInfo.value = lastInfoValue;
	lastInfo.serverId = serverInfo.id;

	LastInfoQueryOption option(USER_ID_SYSTEM);
	option.setLastInfoType(type);
	LastInfoDefList lastInfoList;
	dbLastInfo.getLastInfoList(lastInfoList, option);
	if (lastInfoList.empty())
		dbLastInfo.addLastInfo(lastInfo, privilege);
	else
		dbLastInfo.updateLastInfo(lastInfo, privilege);
}
