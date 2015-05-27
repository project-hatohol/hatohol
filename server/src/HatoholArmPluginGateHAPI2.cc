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

#include <StringUtils.h>
#include <JSONParser.h>
#include <HatoholArmPluginInterfaceHAPI2.h>
#include "HatoholArmPluginGateHAPI2.h"
#include "ThreadLocalDBCache.h"
#include "UnifiedDataStore.h"
#include "ArmFake.h"

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

struct HatoholArmPluginGateHAPI2::Impl
{
	// We have a copy. The access to the object is MT-safe.
	const MonitoringServerInfo m_serverInfo;
	ArmPluginInfo m_pluginInfo;
	ArmFake m_armFake;
	ArmStatus m_armStatus;
	HostInfoCache hostInfoCache;

	Impl(const MonitoringServerInfo &_serverInfo,
	     HatoholArmPluginGateHAPI2 *hapghapi)
	: m_serverInfo(_serverInfo),
	  m_armFake(m_serverInfo),
	  m_armStatus(),
	  hostInfoCache(&_serverInfo.id)
	{
		ArmPluginInfo::initialize(m_pluginInfo);
		ThreadLocalDBCache cache;
		DBTablesConfig &dbConfig = cache.getConfig();
		const ServerIdType &serverId = m_serverInfo.id;
		if (!dbConfig.getArmPluginInfo(m_pluginInfo, serverId)) {
			MLPL_ERR("Failed to get ArmPluginInfo: serverId: %d\n",
				 serverId);
			return;
		}
		hapghapi->setArmPluginInfo(m_pluginInfo);
	}

	~Impl()
	{
		m_armStatus.setRunningStatus(false);
	}

	void start(void)
	{
		m_armStatus.setRunningStatus(true);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateHAPI2::HatoholArmPluginGateHAPI2(
  const MonitoringServerInfo &serverInfo, const bool &autoStart)
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
	  HAPI2_UPDATE_HOST_GROUPS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostGroups);
	registerProcedureHandler(
	  HAPI2_UPDATE_HOST_GROUP_MEMEBRSHIP,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostGroupMembership);
	registerProcedureHandler(
	  HAPI2_UPDATE_TRIGGERS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateTriggers);
	registerProcedureHandler(
	  HAPI2_UPDATE_EVENTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateEvents);
	registerProcedureHandler(
	  HAPI2_UPDATE_HOST_PARENTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostParents);
	registerProcedureHandler(
	  HAPI2_UPDATE_ARM_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateArmInfo);

	if (autoStart)
		m_impl->start();
}

void HatoholArmPluginGateHAPI2::start(void)
{
	HatoholArmPluginInterfaceHAPI2::start();
	m_impl->start();
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

bool HatoholArmPluginGateHAPI2::isFetchItemsSupported(void)
{
	// TODO: implement
	return false;
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchItem(Closure0 *closure)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "fetchItems");
	agent.startObject("params");
	if (false) { // TODO: Pass requested hostIds
		agent.startArray("hostIds");
		agent.endArray();
	}
	// TODO: Use random number and keep it until receiving a response
	agent.add("fetchId", "1");
	agent.endObject();
	agent.add("id", 1); // TODO: Use random number
	agent.endObject();
	send(agent.generate());
	return true;
}

void HatoholArmPluginGateHAPI2::startOnDemandFetchHistory(
  const ItemInfo &itemInfo, const time_t &beginTime, const time_t &endTime,
  Closure1<HistoryInfoVect> *closure)
{
	// TODO: implement
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchTrigger(Closure0 *closure)
{
	// TODO: implement
	return false;
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

static bool parseExchangeProfileParams(JSONParser &parser,
				       ValidProcedureNameVect &validProcedureNameVect)
{
	parser.startObject("procedures");
	size_t num = parser.countElements();
	for (size_t i = 0; i < num; i++) {
		string validProcedureName;
		parser.read(i, validProcedureName);

		validProcedureNameVect.push_back(validProcedureName);
	}
	parser.endObject(); // procedures
	return true;
}

static bool hapProcessLogger(JSONParser &parser)
{
	string hapProcessName;
	parser.read("name", hapProcessName);
	MLPL_INFO("HAP Process connecting done. Connected HAP process name: \"%s\"\n",
		  hapProcessName.c_str());
	return true;
}

string HatoholArmPluginGateHAPI2::procedureHandlerExchangeProfile(
  const HAPI2ProcedureType type, const string &params)
{
	ValidProcedureNameVect validProcedureNameVect;
	JSONParser parser(params);
	int64_t rpcId;
	parser.startObject("params");
	bool succeeded = parseExchangeProfileParams(parser, validProcedureNameVect);
	hapProcessLogger(parser);
	parser.endObject(); // params
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.startObject("result");
	agent.add("name", "exampleName"); // TODO: add process name mechanism
	agent.startArray("procedures");
	for (auto defaultValidProcedureDef : defaultValidProcedureList) {
		if (defaultValidProcedureDef.type == PROCEDURE_BOTH ||
		    defaultValidProcedureDef.type == PROCEDURE_HAP)
			continue;
		agent.add(defaultValidProcedureDef.name);
	}
	agent.endArray(); // procedures
	agent.endObject(); // result
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

string HatoholArmPluginGateHAPI2::procedureHandlerMonitoringServerInfo(
  const HAPI2ProcedureType type, const string &params)
{
	JSONParser parser(params);
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.startObject("result");
	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	agent.add("serverId", serverInfo.id);
	agent.add("url", serverInfo.baseURL);
	agent.add("type", m_impl->m_pluginInfo.uuid);
	agent.add("nickName", serverInfo.nickname);
	agent.add("userName", serverInfo.userName);
	agent.add("password", serverInfo.password);
	agent.add("pollingIntervalSec", serverInfo.pollingIntervalSec);
	agent.add("retryIntervalSec", serverInfo.retryIntervalSec);
	agent.add("extendedInfo", serverInfo.extendedInfo);
	agent.endObject(); // result
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

static bool parseLastInfoParams(JSONParser &parser, LastInfoType &lastInfoType)
{
	string type;
	parser.read("params", type);
	if (type == "host")
		lastInfoType = LAST_INFO_HOST;
	else if (type == "hostGroup")
		lastInfoType = LAST_INFO_HOST_GROUP;
	else if (type == "hostGroupMembership")
		lastInfoType = LAST_INFO_HOST_GROUP_MEMBERSHIP;
	else if (type == "trigger")
		lastInfoType = LAST_INFO_TRIGGER;
	else if (type == "event")
		lastInfoType = LAST_INFO_EVENT;
	else if (type == "hostParent")
		lastInfoType = LAST_INFO_HOST_PARENT;
	else
		lastInfoType = LAST_INFO_ALL; // TODO: should return an error

	return true;
}

string HatoholArmPluginGateHAPI2::procedureHandlerLastInfo(
  const HAPI2ProcedureType type, const string &params)
{
	ThreadLocalDBCache cache;
	DBTablesLastInfo &dbLastInfo = cache.getLastInfo();
	LastInfoQueryOption option(USER_ID_SYSTEM);
	LastInfoType lastInfoType;
	JSONParser parser(params);
	bool succeeded = parseLastInfoParams(parser, lastInfoType);
	int64_t rpcId;
	parser.read("id", rpcId);

	option.setLastInfoType(lastInfoType);
	option.setTargetServerId(m_impl->m_serverInfo.id);
	LastInfoDefList lastInfoList;
	dbLastInfo.getLastInfoList(lastInfoList, option);
	string lastInfoValue = "";
	for (auto lastInfo : lastInfoList) {
		lastInfoValue = lastInfo.value;
		break;
	}

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", lastInfoValue);
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

static bool parseItemParams(JSONParser &parser, ItemInfoList &itemInfoList,
			    const MonitoringServerInfo &serverInfo)
{
	parser.startObject("items");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse item contents.\n");
			return false;
		}

		ItemInfo itemInfo;
		itemInfo.serverId = serverInfo.id;
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
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutItems(
  const HAPI2ProcedureType type, const string &params)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ItemInfoList itemList;
	JSONParser parser(params);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseItemParams(parser, itemList, serverInfo);
	dataStore->addItemList(itemList);

	string fetchId;
	parser.read("fetchId", fetchId);
	// TODO: callback

	parser.endObject(); // params
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", "");
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

static bool parseHistoryParams(JSONParser &parser, HistoryInfoVect &historyInfoVect,
			       const MonitoringServerInfo &serverInfo)
{
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
		historyInfo.serverId = serverInfo.id;
		parser.read("value", historyInfo.value);
		parseTimeStamp(parser, "time", historyInfo.clock);
		parser.endElement();

		historyInfoVect.push_back(historyInfo);
	}
	parser.endObject(); // histories
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHistory(
  const HAPI2ProcedureType type, const string &params)
{
	HistoryInfoVect historyInfoVect;
	JSONParser parser(params);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseHistoryParams(parser, historyInfoVect, serverInfo);
	// TODO: Store or transport historyInfoVect

	string fetchId;
	parser.read("fetchId", fetchId);
	// TODO: callback

	parser.endObject(); // params
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", "");
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

static bool parseHostsParams(JSONParser &parser, ServerHostDefVect &hostInfoVect,
			     const MonitoringServerInfo &serverInfo)
{
	parser.startObject("hosts");
	size_t num = parser.countElements();
	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			return false;
		}

		ServerHostDef hostInfo;
		hostInfo.serverId = serverInfo.id;
		int64_t hostId;
		parser.read("hostId", hostId);
		hostInfo.hostIdInServer = hostId;
		parser.read("hostName", hostInfo.name);
		parser.endElement();

		hostInfoVect.push_back(hostInfo);
	}
	parser.endObject(); // hosts
	return true;
};

static bool parseUpdateType(JSONParser &parser, string &updateType)
{
	parser.read("updateType", updateType);
	if (updateType == "ALL") {
		return true;
	} else if (updateType == "UPDATED") {
		return false;
	} else {
		MLPL_WARN("Invalid update type: %s\n", updateType.c_str());
		return false;
	}
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateHosts(
  const HAPI2ProcedureType type, const string &params)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerHostDefVect hostInfoVect;
	JSONParser parser(params);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseHostsParams(parser, hostInfoVect, serverInfo);
	string updateType;
	bool checkInvalidHosts = parseUpdateType(parser, updateType);
	// TODO: implement validation for Hosts
	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		upsertLastInfo(lastInfo, LAST_INFO_HOST);
	}

	dataStore->upsertHosts(hostInfoVect);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	parser.endObject(); // params
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", result);
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

static bool parseHostGroupsParams(JSONParser &parser,
				  HostgroupVect &hostgroupVect,
				  const MonitoringServerInfo &serverInfo)
{
	parser.startObject("hostGroups");
	size_t num = parser.countElements();
	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			return false;
		}

		Hostgroup hostgroup;
		hostgroup.serverId = serverInfo.id;
		parser.read("groupId", hostgroup.idInServer);
		parser.read("groupName", hostgroup.name);
		parser.endElement();

		hostgroupVect.push_back(hostgroup);
	}
	parser.endObject(); // hostsGroups
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostGroups(
  const HAPI2ProcedureType type, const string &params)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupVect hostgroupVect;
	JSONParser parser(params);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseHostGroupsParams(parser, hostgroupVect, serverInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidHostGroups = parseUpdateType(parser, updateType);
	// TODO: implement validation for HostGroups
	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		upsertLastInfo(lastInfo, LAST_INFO_HOST_GROUP);
	}
	dataStore->upsertHostgroups(hostgroupVect);

	parser.endObject(); // params
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", result);
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}


static bool parseHostGroupMembershipParams(
  JSONParser &parser,
  HostgroupMemberVect &hostgroupMemberVect,
  const MonitoringServerInfo &serverInfo)
{
	parser.startObject("hostGroupsMembership");
	size_t num = parser.countElements();
	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			return false;
		}

		HostgroupMember hostgroupMember;
		hostgroupMember.serverId = serverInfo.id;
		string hostId;
		parser.read("hostId", hostId);
		hostgroupMember.hostId = StringUtils::toUint64(hostId);
		parser.startObject("groupIds");
		size_t groupIdNum = parser.countElements();
		for (size_t j = 0; j < groupIdNum; j++) {
			parser.read(j, hostgroupMember.hostgroupIdInServer);
			hostgroupMemberVect.push_back(hostgroupMember);
		}
		parser.endObject(); // groupIds
		parser.endElement();
	}
	parser.endObject(); // hostGroupsMembership
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostGroupMembership(
  const HAPI2ProcedureType type, const string &params)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupMemberVect hostgroupMembershipVect;
	JSONParser parser(params);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseHostGroupMembershipParams(parser, hostgroupMembershipVect,
							serverInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidHostGroupMembership =
	  parseUpdateType(parser, updateType);
	// TODO: implement validation for HostGroupMembership
	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		upsertLastInfo(lastInfo, LAST_INFO_HOST_GROUP_MEMBERSHIP);
	}
	dataStore->upsertHostgroupMembers(hostgroupMembershipVect);

	parser.endObject(); // params
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", result);
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

static void parseTriggerStatus(JSONParser &parser, TriggerStatusType &status)
{
	string statusString;
	parser.read("status", statusString);
	if (statusString == "OK") {
		status = TRIGGER_STATUS_OK;
	} else if (statusString == "NG") {
		status = TRIGGER_STATUS_PROBLEM;
	} else if (statusString == "UNKNOWN") {
		status = TRIGGER_STATUS_UNKNOWN;
	} else {
		MLPL_WARN("Unknown trigger status: %s\n", statusString.c_str());
		status = TRIGGER_STATUS_UNKNOWN;
	}
}

static void parseTriggerSeverity(JSONParser &parser,
				 TriggerSeverityType &severity)
{
	string severityString;
	parser.read("severity", severityString);
	if (severityString == "ALL") {
		severity = TRIGGER_SEVERITY_ALL;
	} else if (severityString == "UNKNOWN") {
		severity = TRIGGER_SEVERITY_UNKNOWN;
	} else if (severityString == "INFO") {
		severity = TRIGGER_SEVERITY_INFO;
	} else if (severityString == "WARNING") {
		severity = TRIGGER_SEVERITY_WARNING;
	} else if (severityString == "ERROR") {
		severity = TRIGGER_SEVERITY_ERROR;
	} else if (severityString == "CRITICAL") {
		severity = TRIGGER_SEVERITY_CRITICAL;
	} else if (severityString == "EMERGENCY") {
		severity = TRIGGER_SEVERITY_EMERGENCY;
	} else {
		MLPL_WARN("Unknown trigger severity: %s\n",
			  severityString.c_str());
		severity = TRIGGER_SEVERITY_UNKNOWN;
	}
}

static bool parseTriggersParams(JSONParser &parser, TriggerInfoList &triggerInfoList,
				const MonitoringServerInfo &serverInfo)
{
	parser.startObject("triggers");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse event contents.\n");
			return false;
		}

		TriggerInfo triggerInfo;
		triggerInfo.serverId = serverInfo.id;
		parseTriggerStatus(parser, triggerInfo.status);
		parseTriggerSeverity(parser, triggerInfo.severity);
		parseTimeStamp(parser, "lastChangeTime", triggerInfo.lastChangeTime);
		parser.read("hostId",       triggerInfo.hostIdInServer);
		parser.read("hostName",     triggerInfo.hostName);
		parser.read("brief",        triggerInfo.brief);
		parser.read("extendedInfo", triggerInfo.extendedInfo);
		parser.endElement();

		triggerInfoList.push_back(triggerInfo);
	}
	parser.endObject(); // triggers
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateTriggers(
  const HAPI2ProcedureType type, const string &params)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	TriggerInfoList triggerInfoList;
	JSONParser parser(params);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseTriggersParams(parser, triggerInfoList, serverInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidTriggers = parseUpdateType(parser, updateType);
	// TODO: implement validation for triggers
	dbMonitoring.addTriggerInfoList(triggerInfoList);
	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_TRIGGER);
	}

	if (parser.isMember("mayMoreFlag")) {
		bool mayMoreFlag;
		parser.read("mayMoreFlag", mayMoreFlag);
		// TODO: What should we do?
	}
	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		// TODO: callback
	}

	parser.endObject(); // params
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", result);
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

static void parseEventType(JSONParser &parser, EventInfo &eventInfo)
{
	string eventType;
	parser.read("type", eventType);

	if (eventType == "GOOD") {
		eventInfo.type = EVENT_TYPE_GOOD;
	} else if (eventType == "BAD") {
		eventInfo.type = EVENT_TYPE_BAD;
	} else if (eventType == "UNKNOWN") {
		eventInfo.type = EVENT_TYPE_UNKNOWN;
	} else if (eventType == "NOTIFICATION") {
		// TODO: Add EVENT_TYPE_NOTIFICATION
		//eventInfo.type = EVENT_TYPE_NOTIFICATION;
	} else {
		MLPL_WARN("Invalid event type: %s\n", eventType.c_str());
		eventInfo.type = EVENT_TYPE_UNKNOWN;
	}
};

static bool parseEventsParams(JSONParser &parser, EventInfoList &eventInfoList,
			      const MonitoringServerInfo &serverInfo)
{
	parser.startObject("events");
	size_t num = parser.countElements();
	constexpr const size_t numLimit = 1000;
	static const TriggerIdType DO_NOT_ASSOCIATE_TRIGGER_ID =
		SPECIAL_TRIGGER_ID_PREFIX "DO_NOT_ASSOCIATE_TRIGGER";

	if (num > numLimit) {
		MLPL_ERR("Event Object is too large. "
			 "Object size limit(%zd) exceeded.\n", numLimit);
		return false;
	}

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse event contents.\n");
			return false;
		}

		EventInfo eventInfo;
		eventInfo.serverId = serverInfo.id;
		parser.read("eventId",      eventInfo.id);
		parseTimeStamp(parser, "time", eventInfo.time);
		parseEventType(parser, eventInfo);
		TriggerIdType triggerId = DO_NOT_ASSOCIATE_TRIGGER_ID;
		if (!parser.read("triggerId", triggerId)) {
			eventInfo.triggerId = triggerId;
		}
		parseTriggerStatus(parser, eventInfo.status);
		parseTriggerSeverity(parser, eventInfo.severity);
		parser.read("hostId",       eventInfo.hostIdInServer);
		parser.read("hostName",     eventInfo.hostName);
		parser.read("brief",        eventInfo.brief);
		parser.read("extendedInfo", eventInfo.extendedInfo);
		parser.endElement();

		eventInfoList.push_back(eventInfo);
	}
	parser.endObject(); // events
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateEvents(
  const HAPI2ProcedureType type, const string &params)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList eventInfoList;
	JSONParser parser(params);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseEventsParams(parser, eventInfoList, serverInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";
	dataStore->addEventList(eventInfoList);
	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_EVENT);
	}

	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		// TODO: callback
	}

	parser.endObject(); // params
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", result);
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

static bool parseHostParentsParams(
  JSONParser &parser,
  VMInfoVect &vmInfoVect)
{
	parser.startObject("hostParents");
	size_t num = parser.countElements();
	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse vm_info contents.\n");
			return false;
		}

		VMInfo vmInfo;
		string childHostId, parentHostId;
		parser.read("childHostId", childHostId);
		vmInfo.hostId = StringUtils::toUint64(childHostId);
		parser.read("parentHostId", parentHostId);
		vmInfo.hypervisorHostId = StringUtils::toUint64(parentHostId);
		parser.endElement();

		vmInfoVect.push_back(vmInfo);
	}
	parser.endObject(); // hostsParents
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostParents(
  const HAPI2ProcedureType type, const string &params)
{
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	VMInfoVect vmInfoVect;
	JSONParser parser(params);
	parser.startObject("params");

	bool succeeded = parseHostParentsParams(parser, vmInfoVect);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidTriggers = parseUpdateType(parser, updateType);
	// TODO: implement validation for hostParents
	for (auto vmInfo : vmInfoVect)
		dbHost.upsertVMInfo(vmInfo);
	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_HOST_PARENT);
	}

	parser.endObject(); // params
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", result);
	agent.add("id", rpcId);
	agent.endObject();
	return agent.generate();
}

static bool parseArmInfoParams(JSONParser &parser, ArmInfo &armInfo)
{
	string status;
	parser.read("lastStatus", status);
	if (status == "INIT") {
		armInfo.stat = ARM_WORK_STAT_INIT;
	} else if (status == "OK") {
		armInfo.stat = ARM_WORK_STAT_OK;
	} else if (status == "NG") {
		armInfo.stat = ARM_WORK_STAT_FAILURE;
	} else {
		MLPL_WARN("Invalid status: %s\n", status.c_str());
		armInfo.stat = ARM_WORK_STAT_FAILURE;
	}
	parser.read("failureReason", armInfo.failureComment);
	timespec successTime, failureTime;
	parseTimeStamp(parser, "lastSuccessTime", successTime);
	parseTimeStamp(parser, "lastFailureTime", failureTime);
	SmartTime lastSuccessTime(successTime);
	SmartTime lastFailureTime(failureTime);
	armInfo.statUpdateTime = lastSuccessTime;
	armInfo.lastFailureTime = lastFailureTime;

	int64_t numSuccess, numFailure;
	parser.read("numSuccess", numSuccess);
	parser.read("numFailure", numFailure);
	armInfo.numUpdate = (size_t)numSuccess;
	armInfo.numFailure = (size_t)numFailure;

	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateArmInfo(
  const HAPI2ProcedureType type, const string &params)
{
	ArmStatus status;
	ArmInfo armInfo;
	JSONParser parser(params);
	parser.startObject("params");

	bool succeeded = parseArmInfoParams(parser, armInfo);
	status.setArmInfo(armInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	parser.endObject(); // params
	int64_t rpcId;
	parser.read("id", rpcId);

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("result", result);
	agent.add("id", rpcId);
	agent.endObject();
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
	lastInfo.id = AUTO_INCREMENT_VALUE;
	lastInfo.dataType = type;
	lastInfo.value = lastInfoValue;
	lastInfo.serverId = serverInfo.id;
	dbLastInfo.upsertLastInfo(lastInfo, privilege);
}

mt19937 HatoholArmPluginGateHAPI2::getRandomEngine(void)
{
	std::random_device rd;
	std::mt19937 engine(rd());
	return engine;
}
