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

struct JSONRPCErrorObject {
	StringList errors;
	void addError(const char *format,
		      ...) __attribute__((__format__ (__printf__, 2, 3)))
	{
	       va_list ap;
	       va_start(ap, format);
	       using namespace mlpl::StringUtils;
	       errors.push_back(vsprintf(format, ap));
	       va_end(ap);
	}

	bool hasErrors() {
		return !errors.empty();
	}

	StringList getErrors() {
		return errors;
	}
};

#define PARSE_AS_MANDATORY(MEMBER, VALUE, RPCERR)				\
if (!parser.read(MEMBER, VALUE)) {						\
	MLPL_ERR("Failed to parse '%s' member.\n", MEMBER);			\
	string errorMessage =							\
		"Failed to parse mandatory member:";				\
	RPCERR.addError("%s '%s' does not exist.",				\
			errorMessage.c_str(), MEMBER);				\
}

#define CHECK_MANDATORY_PARAMS_EXISTENCE(PARAMS, RPCERR)			\
if (!parser.isMember(PARAMS)) {						\
	MLPL_ERR("Failed to parse '%s'.\n", PARAMS);			\
	string errorMessage = "Failed to parse:";				\
	RPCERR.addError("%s '%s' does not exist.",				\
			errorMessage.c_str(), PARAMS);				\
}

#define CHECK_MANDATORY_OBJECT_EXISTENCE(MEMBER, RPCERR)			\
if (!parser.isMember(MEMBER)) {						\
	MLPL_ERR("Failed to parse mandatory object '%s'.\n", MEMBER);		\
	string errorMessage =							\
		"Failed to parse mandatory object: ";				\
	RPCERR.addError("%s '%s' does not exist.",				\
			errorMessage.c_str(), MEMBER);				\
	return false;								\
}

struct HatoholArmPluginGateHAPI2::Impl
{
	// We have a copy. The access to the object is MT-safe.
	const MonitoringServerInfo m_serverInfo;
	HatoholArmPluginGateHAPI2 &m_hapi2;
	ArmPluginInfo m_pluginInfo;
	ArmFake m_armFake;
	ArmStatus m_armStatus;
	string m_pluginProcessName;
	set<string> m_supportedProcedureNameSet;
	HostInfoCache hostInfoCache;
	map<string, Closure0 *> m_fetchClosureMap;
	map<string, Closure1<HistoryInfoVect> *> m_fetchHistoryClosureMap;

	Impl(const MonitoringServerInfo &_serverInfo,
	     HatoholArmPluginGateHAPI2 &hapi2)
	: m_serverInfo(_serverInfo),
	  m_hapi2(hapi2),
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
		m_hapi2.setArmPluginInfo(m_pluginInfo);
	}

	~Impl()
	{
		for (auto pair: m_fetchClosureMap) {
			Closure0 *closure = pair.second;
			if (closure)
				(*closure)();
			delete closure;
		}
		for (auto pair: m_fetchHistoryClosureMap) {
			Closure1<HistoryInfoVect> *closure = pair.second;
			HistoryInfoVect historyInfoVect;
			if (closure)
				(*closure)(historyInfoVect);
			delete closure;
		}
		m_armStatus.setRunningStatus(false);
	}

	void start(void)
	{
		m_armStatus.setRunningStatus(true);
		callExchangeProfile();
	}

	bool parseExchangeProfileParams(JSONParser &parser)
	{
		JSONRPCErrorObject errObj;
		m_supportedProcedureNameSet.clear();
		parser.startObject("procedures");
		size_t num = parser.countElements();
		for (size_t i = 0; i < num; i++) {
			string supportedProcedureName;
			parser.read(i, supportedProcedureName);
			m_supportedProcedureNameSet.insert(supportedProcedureName);
		}
		parser.endObject(); // procedures

		PARSE_AS_MANDATORY("name", m_pluginProcessName, errObj);
		MLPL_INFO("HAP Process connecting done. "
			  "Connected HAP process name: \"%s\"\n",
			  m_pluginProcessName.c_str());
		return true;
	}

	struct ExchangeProfileCallback : public ProcedureCallback {
		Impl &m_impl;
		ExchangeProfileCallback(Impl &impl)
		: m_impl(impl)
		{
		}

		virtual void onGotResponse(JSONParser &parser) override
		{
			if (parser.isMember("error")) {
				string errorMessage;
				parser.startObject("error");
				parser.read("message", errorMessage);
				parser.endObject();
				MLPL_WARN("Received an error on calling "
					  "exchangeProfile: %s\n",
					  errorMessage.c_str());
				return;
			}

			parser.startObject("result");
			m_impl.parseExchangeProfileParams(parser);
			parser.endObject();
			m_impl.m_hapi2.setEstablished(true);
		}
	};

	void callExchangeProfile(void) {
		JSONBuilder builder;
		builder.startObject();
		builder.add("jsonrpc", "2.0");
		builder.add("name", PACKAGE_NAME);
		builder.add("method", HAPI2_EXCHANGE_PROFILE);
		builder.startObject("params");
		builder.startArray("procedures");
		for (auto procedureDef : m_hapi2.getProcedureDefList()) {
			if (procedureDef.type == PROCEDURE_HAP)
				continue;
			builder.add(procedureDef.name);
		}
		builder.endArray(); // procedures
		builder.endObject(); // params
		std::mt19937 random = m_hapi2.getRandomEngine();
		int64_t id = random();
		builder.add("id", id);
		builder.endObject();
		ProcedureCallback *callback =
		  new Impl::ExchangeProfileCallback(*this);
		ProcedureCallbackPtr callbackPtr(callback, false);
		m_hapi2.send(builder.generate(), id, callback);
	}

	void queueFetchCallback(const string &fetchId, Closure0 *closure)
	{
		if (fetchId.empty())
			return;
		m_fetchClosureMap[fetchId] = closure;
	}

	void runFetchCallback(const string &fetchId)
	{
		if (fetchId.empty())
			return;

		auto it = m_fetchClosureMap.find(fetchId);
		if (it != m_fetchClosureMap.end()) {
			Closure0 *closure = it->second;
			if (closure)
				(*closure)();
			m_fetchClosureMap.erase(it);
			delete closure;
		}
	}

	void queueFetchHistoryCallback(const string &fetchId,
				       Closure1<HistoryInfoVect> *closure)
	{
		if (fetchId.empty())
			return;
		m_fetchHistoryClosureMap[fetchId] = closure;
	}

	void runFetchHistoryCallback(const string &fetchId,
				     const HistoryInfoVect &historyInfoVect)
	{
		if (fetchId.empty())
			return;

		auto it = m_fetchHistoryClosureMap.find(fetchId);
		if (it != m_fetchHistoryClosureMap.end()) {
			Closure1<HistoryInfoVect> *closure = it->second;
			(*closure)(historyInfoVect);
			m_fetchHistoryClosureMap.erase(it);
			delete closure;
		}
	}

	bool hasProcedure(const string procedureName)
	{
		return m_supportedProcedureNameSet.find(procedureName) !=
			m_supportedProcedureNameSet.end();
	}

	struct FetchProcedureCallback : public ProcedureCallback {
		Impl &m_impl;
		const string m_fetchId;
		const string m_methodName;
		FetchProcedureCallback(Impl &impl,
				       const string &fetchId,
				       const string &methodName)
		: m_impl(impl), m_fetchId(fetchId), m_methodName(methodName)
		{
		}

		bool isSucceeded(JSONParser &parser)
		{
			if (parser.isMember("error"))
				return false;

			string result;
			parser.read("result", result);
			return result == "SUCCESS";
		}

		virtual void onGotResponse(JSONParser &parser) override
		{
			if (isSucceeded(parser)) {
				// The callback function will be executed on
				// put* or update* procedures.
				return;
			}

			// The fetch* procedure has not been accepted by the
			// plugin. The closure for it should be expired
			// immediately.
			if (m_methodName == HAPI2_FETCH_HISTORY) {
				HistoryInfoVect historyInfoVect;
				m_impl.runFetchHistoryCallback(m_fetchId,
							       historyInfoVect);
			} else {
				m_impl.runFetchCallback(m_fetchId);
			}

			// TODO: output error log
		}
	};
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateHAPI2::HatoholArmPluginGateHAPI2(
  const MonitoringServerInfo &serverInfo, const bool &autoStart)
: HatoholArmPluginInterfaceHAPI2(MODE_SERVER),
  m_impl(new Impl(serverInfo, *this))
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
	  HAPI2_PUT_HOSTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHosts);
	registerProcedureHandler(
	  HAPI2_PUT_HOST_GROUPS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHostGroups);
	registerProcedureHandler(
	  HAPI2_PUT_HOST_GROUP_MEMEBRSHIP,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHostGroupMembership);
	registerProcedureHandler(
	  HAPI2_PUT_TRIGGERS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutTriggers);
	registerProcedureHandler(
	  HAPI2_PUT_EVENTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutEvents);
	registerProcedureHandler(
	  HAPI2_PUT_HOST_PARENTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHostParents);
	registerProcedureHandler(
	  HAPI2_PUT_ARM_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutArmInfo);

	if (autoStart)
		start();
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
	return m_impl->hasProcedure(HAPI2_FETCH_ITEMS);
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchItem(Closure0 *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_ITEMS))
		return false;

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_ITEMS);
	builder.startObject("params");
	if (false) { // TODO: Pass requested hostIds
		builder.startArray("hostIds");
		builder.endArray();
	}
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_ITEMS);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
	return true;
}

string buildTimeStamp(const time_t &timeValue)
{
	struct tm tm;;
	gmtime_r(&timeValue, &tm);
	char buf[32];
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &tm);
	string timeString;
	timeString = buf;
	return timeString;
}

void HatoholArmPluginGateHAPI2::startOnDemandFetchHistory(
  const ItemInfo &itemInfo, const time_t &beginTime, const time_t &endTime,
  Closure1<HistoryInfoVect> *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_HISTORY)) {
		HistoryInfoVect historyInfoVect;
		if (closure)
			(*closure)(historyInfoVect);
		delete closure;
		return;
	}

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_HISTORY);
	builder.startObject("params");
	builder.add("hostId", itemInfo.hostIdInServer);
	builder.add("itemId", itemInfo.id);
	builder.add("beginTime", buildTimeStamp(beginTime));
	builder.add("endTime", buildTimeStamp(endTime));
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchHistoryCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_HISTORY);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchTrigger(Closure0 *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_TRIGGERS))
		return false;

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_TRIGGERS);
	builder.startObject("params");
	if (false) { // TODO: Pass requested hostIds
		builder.startArray("hostIds");
		builder.endArray();
	}
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_TRIGGERS);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
	return true;
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchEvents(
  Closure0 *closure, const std::string &lastInfo, const size_t count,
  const bool ascending)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_EVENTS))
		return false;

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_EVENTS);
	builder.startObject("params");
	builder.add("lastInfo", lastInfo);
	builder.add("count", count);
	builder.add("direction", ascending ? "ASC" : "DESC");
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_EVENTS);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
	return true;
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
  JSONParser &parser)
{
	m_impl->m_supportedProcedureNameSet.clear();
	parser.startObject("params");
	m_impl->parseExchangeProfileParams(parser);
	parser.endObject(); // params
	setEstablished(true);

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.startObject("result");
	builder.add("name", PACKAGE_NAME);
	builder.startArray("procedures");
	for (auto procedureDef : getProcedureDefList()) {
		if (procedureDef.type == PROCEDURE_HAP)
			continue;
		builder.add(procedureDef.name);
	}
	builder.endArray(); // procedures
	builder.endObject(); // result
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

string HatoholArmPluginGateHAPI2::procedureHandlerMonitoringServerInfo(
  JSONParser &parser)
{
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.startObject("result");
	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	builder.add("serverId", serverInfo.id);
	builder.add("url", serverInfo.baseURL);
	builder.add("type", m_impl->m_pluginInfo.uuid);
	builder.add("nickName", serverInfo.nickname);
	builder.add("userName", serverInfo.userName);
	builder.add("password", serverInfo.password);
	builder.add("pollingIntervalSec", serverInfo.pollingIntervalSec);
	builder.add("retryIntervalSec", serverInfo.retryIntervalSec);
	builder.add("extendedInfo", serverInfo.extendedInfo);
	builder.endObject(); // result
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseLastInfoParams(JSONParser &parser, LastInfoType &lastInfoType,
				JSONRPCErrorObject &errObj)
{
	string type;
	PARSE_AS_MANDATORY("params", type, errObj);
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
	else {
		lastInfoType = LAST_INFO_ALL;
		return false;
	}

	return true;
}

string HatoholArmPluginGateHAPI2::procedureHandlerLastInfo(JSONParser &parser)
{
	ThreadLocalDBCache cache;
	DBTablesLastInfo &dbLastInfo = cache.getLastInfo();
	LastInfoQueryOption option(USER_ID_SYSTEM);
	LastInfoType lastInfoType;
	JSONRPCErrorObject errObj;
	bool succeeded = parseLastInfoParams(parser, lastInfoType, errObj);

	if (!succeeded) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}
	option.setLastInfoType(lastInfoType);
	option.setTargetServerId(m_impl->m_serverInfo.id);
	LastInfoDefList lastInfoList;
	dbLastInfo.getLastInfoList(lastInfoList, option);
	string lastInfoValue = "";
	for (auto lastInfo : lastInfoList) {
		lastInfoValue = lastInfo.value;
		break;
	}

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", lastInfoValue);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseItemParams(JSONParser &parser, ItemInfoList &itemInfoList,
			    const MonitoringServerInfo &serverInfo,
			    JSONRPCErrorObject &errObj)
{
	CHECK_MANDATORY_OBJECT_EXISTENCE("items", errObj);
	parser.startObject("items");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse item contents.\n");
			return false;
		}

		ItemInfo itemInfo;
		itemInfo.serverId = serverInfo.id;
		PARSE_AS_MANDATORY("itemId", itemInfo.id, errObj);
		PARSE_AS_MANDATORY("hostId", itemInfo.hostIdInServer, errObj);
		PARSE_AS_MANDATORY("brief", itemInfo.brief, errObj);
		parseTimeStamp(parser, "lastValueTime", itemInfo.lastValueTime);
		PARSE_AS_MANDATORY("lastValue", itemInfo.lastValue, errObj);
		PARSE_AS_MANDATORY("itemGroupName", itemInfo.itemGroupName, errObj);
		PARSE_AS_MANDATORY("unit", itemInfo.unit, errObj);
		parser.endElement();

		itemInfoList.push_back(itemInfo);
	}
	parser.endObject(); // items
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutItems(JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ItemInfoList itemList;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	JSONRPCErrorObject errObj;
	bool succeeded = parseItemParams(parser, itemList, serverInfo, errObj);
	if (!succeeded) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}
	dataStore->addItemList(itemList);
	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		m_impl->runFetchCallback(fetchId);
	}
	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", "");
	setResponseId(parser, builder);
	builder.endObject();

	return builder.generate();
}

static bool parseHistoryParams(JSONParser &parser, HistoryInfoVect &historyInfoVect,
			       const MonitoringServerInfo &serverInfo,
			       JSONRPCErrorObject &errObj
)
{
	ItemIdType itemId = "";
	PARSE_AS_MANDATORY("itemId", itemId, errObj);
	CHECK_MANDATORY_OBJECT_EXISTENCE("histories", errObj);
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
		PARSE_AS_MANDATORY("value", historyInfo.value, errObj);
		parseTimeStamp(parser, "time", historyInfo.clock);
		parser.endElement();

		historyInfoVect.push_back(historyInfo);
	}
	parser.endObject(); // histories
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHistory(
  JSONParser &parser)
{
	HistoryInfoVect historyInfoVect;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	JSONRPCErrorObject errObj;
	bool succeeded = parseHistoryParams(parser, historyInfoVect,
					    serverInfo, errObj);
	if (!succeeded) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}
	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		m_impl->runFetchHistoryCallback(fetchId, historyInfoVect);
	}

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", "");
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseHostsParams(JSONParser &parser, ServerHostDefVect &hostInfoVect,
			     const MonitoringServerInfo &serverInfo,
			     JSONRPCErrorObject &errObj)
{
	CHECK_MANDATORY_OBJECT_EXISTENCE("hosts", errObj);
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
		PARSE_AS_MANDATORY("hostId", hostId, errObj);
		hostInfo.hostIdInServer = hostId;
		PARSE_AS_MANDATORY("hostName", hostInfo.name, errObj);
		parser.endElement();

		hostInfoVect.push_back(hostInfo);
	}
	parser.endObject(); // hosts
	return true;
};

static bool parseUpdateType(JSONParser &parser, string &updateType,
			    JSONRPCErrorObject &errObj)
{
	PARSE_AS_MANDATORY("updateType", updateType, errObj);
	if (updateType == "ALL") {
		return true;
	} else if (updateType == "UPDATED") {
		return false;
	} else {
		MLPL_WARN("Invalid update type: %s\n", updateType.c_str());
		return false;
	}
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHosts(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerHostDefVect hostInfoVect;
	JSONRPCErrorObject errObj;
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseHostsParams(parser, hostInfoVect, serverInfo, errObj);

	string updateType;
	bool checkInvalidHosts = parseUpdateType(parser, updateType, errObj);

	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		upsertLastInfo(lastInfo, LAST_INFO_HOST);
	}

	string result = succeeded ? "SUCCESS" : "FAILURE";

	parser.endObject(); // params

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}

	// TODO: reflect error in response
	if (checkInvalidHosts) {
		dataStore->syncHosts(hostInfoVect, serverInfo.id,
				     m_impl->hostInfoCache);
	} else {
		dataStore->upsertHosts(hostInfoVect);
	}

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseHostGroupsParams(JSONParser &parser,
				  HostgroupVect &hostgroupVect,
				  const MonitoringServerInfo &serverInfo,
				  JSONRPCErrorObject &errObj)
{
	CHECK_MANDATORY_OBJECT_EXISTENCE("hostGroups", errObj);
	parser.startObject("hostGroups");
	size_t num = parser.countElements();
	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			return false;
		}

		Hostgroup hostgroup;
		hostgroup.serverId = serverInfo.id;
		PARSE_AS_MANDATORY("groupId", hostgroup.idInServer, errObj);
		PARSE_AS_MANDATORY("groupName", hostgroup.name, errObj);
		parser.endElement();

		hostgroupVect.push_back(hostgroup);
	}
	parser.endObject(); // hostsGroups
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHostGroups(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupVect hostgroupVect;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	JSONRPCErrorObject errObj;
	bool succeeded = parseHostGroupsParams(parser, hostgroupVect,
					       serverInfo, errObj);
	if (!succeeded) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidHostGroups = parseUpdateType(parser, updateType, errObj);
	// TODO: reflect error in response
	if (checkInvalidHostGroups) {
		dataStore->syncHostgroups(hostgroupVect, serverInfo.id);
	} else {
		dataStore->upsertHostgroups(hostgroupVect);
	}
	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		upsertLastInfo(lastInfo, LAST_INFO_HOST_GROUP);
	}
	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}


static bool parseHostGroupMembershipParams(
  JSONParser &parser,
  HostgroupMemberVect &hostgroupMemberVect,
  const MonitoringServerInfo &serverInfo,
  JSONRPCErrorObject &errObj
)
{
	CHECK_MANDATORY_OBJECT_EXISTENCE("hostGroupMembership", errObj);
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
		PARSE_AS_MANDATORY("hostId", hostId, errObj);
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

string HatoholArmPluginGateHAPI2::procedureHandlerPutHostGroupMembership(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupMemberVect hostgroupMembershipVect;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	JSONRPCErrorObject errObj;
	bool succeeded = parseHostGroupMembershipParams(parser,
							hostgroupMembershipVect,
							serverInfo, errObj);
	if (!succeeded) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidHostGroupMembership =
		parseUpdateType(parser, updateType, errObj);
	// TODO: reflect error in response
	if (checkInvalidHostGroupMembership) {
		dataStore->syncHostgroupMembers(hostgroupMembershipVect, serverInfo.id);
	} else {
		dataStore->upsertHostgroupMembers(hostgroupMembershipVect);
	}
	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		upsertLastInfo(lastInfo, LAST_INFO_HOST_GROUP_MEMBERSHIP);
	}
	dataStore->upsertHostgroupMembers(hostgroupMembershipVect);

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseTriggerStatus(JSONParser &parser, TriggerStatusType &status,
			       JSONRPCErrorObject &errObj)
{
	string statusString;
	PARSE_AS_MANDATORY("status", statusString, errObj);
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
	return true;
}

static bool parseTriggerSeverity(JSONParser &parser,
				 TriggerSeverityType &severity,
				 JSONRPCErrorObject &errObj)
{
	string severityString;
	PARSE_AS_MANDATORY("severity", severityString, errObj);
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
	return true;
}

static bool parseTriggersParams(JSONParser &parser, TriggerInfoList &triggerInfoList,
				const MonitoringServerInfo &serverInfo,
				JSONRPCErrorObject &errObj)
{
	CHECK_MANDATORY_OBJECT_EXISTENCE("triggers", errObj);
	parser.startObject("triggers");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse event contents.\n");
			return false;
		}

		TriggerInfo triggerInfo;
		triggerInfo.serverId = serverInfo.id;
		parseTriggerStatus(parser, triggerInfo.status, errObj);
		parseTriggerSeverity(parser, triggerInfo.severity, errObj);
		parseTimeStamp(parser, "lastChangeTime", triggerInfo.lastChangeTime);
		PARSE_AS_MANDATORY("hostId",       triggerInfo.hostIdInServer, errObj);
		PARSE_AS_MANDATORY("hostName",     triggerInfo.hostName, errObj);
		PARSE_AS_MANDATORY("brief",        triggerInfo.brief, errObj);
		PARSE_AS_MANDATORY("extendedInfo", triggerInfo.extendedInfo, errObj);
		parser.endElement();

		triggerInfoList.push_back(triggerInfo);
	}
	parser.endObject(); // triggers
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutTriggers(
  JSONParser &parser)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	TriggerInfoList triggerInfoList;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	JSONRPCErrorObject errObj;
	bool succeeded = parseTriggersParams(parser, triggerInfoList,
					     serverInfo, errObj);
	if (!succeeded) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidTriggers = parseUpdateType(parser, updateType, errObj);
	// TODO: reflect error in response
	if (checkInvalidTriggers) {
		dbMonitoring.syncTriggers(triggerInfoList, serverInfo.id);
	} else {
		dbMonitoring.addTriggerInfoList(triggerInfoList);
	}

	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_TRIGGER);
	}

	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		m_impl->runFetchCallback(fetchId);
	}

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseEventType(JSONParser &parser, EventInfo &eventInfo,
			   JSONRPCErrorObject &errObj)
{
	string eventType;
	PARSE_AS_MANDATORY("type", eventType, errObj);

	if (eventType == "GOOD") {
		eventInfo.type = EVENT_TYPE_GOOD;
	} else if (eventType == "BAD") {
		eventInfo.type = EVENT_TYPE_BAD;
	} else if (eventType == "UNKNOWN") {
		eventInfo.type = EVENT_TYPE_UNKNOWN;
	} else if (eventType == "NOTIFICATION") {
		eventInfo.type = EVENT_TYPE_NOTIFICATION;
	} else {
		MLPL_WARN("Invalid event type: %s\n", eventType.c_str());
		eventInfo.type = EVENT_TYPE_UNKNOWN;
	}
	return true;
};

static bool parseEventsParams(JSONParser &parser, EventInfoList &eventInfoList,
			      const MonitoringServerInfo &serverInfo,
			      JSONRPCErrorObject &errObj)
{
	CHECK_MANDATORY_OBJECT_EXISTENCE("events", errObj);
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
		PARSE_AS_MANDATORY("eventId",  eventInfo.id, errObj);
		parseTimeStamp(parser, "time", eventInfo.time);
		parseEventType(parser, eventInfo, errObj);
		TriggerIdType triggerId = DO_NOT_ASSOCIATE_TRIGGER_ID;
		if (!parser.read("triggerId", triggerId)) {
			eventInfo.triggerId = triggerId;
		}
		parseTriggerStatus(parser,         eventInfo.status, errObj);
		parseTriggerSeverity(parser,       eventInfo.severity, errObj);
		PARSE_AS_MANDATORY("hostId",       eventInfo.hostIdInServer, errObj);
		PARSE_AS_MANDATORY("hostName",     eventInfo.hostName, errObj);
		PARSE_AS_MANDATORY("brief",        eventInfo.brief, errObj);
		PARSE_AS_MANDATORY("extendedInfo", eventInfo.extendedInfo, errObj);
		parser.endElement();

		eventInfoList.push_back(eventInfo);
	}
	parser.endObject(); // events
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutEvents(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList eventInfoList;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	JSONRPCErrorObject errObj;
	bool succeeded = parseEventsParams(parser, eventInfoList, serverInfo, errObj);
	if (!succeeded) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}
	string result = succeeded ? "SUCCESS" : "FAILURE";
	dataStore->addEventList(eventInfoList);
	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_EVENT);
	}

	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		bool mayMoreFlag = false;
		if (parser.isMember("mayMoreFlag")) {
			parser.read("mayMoreFlag", mayMoreFlag);
		} else {
			MLPL_WARN("No mayMoreFlag while a fetchId is provided!");
		}
		if (!mayMoreFlag)
			m_impl->runFetchCallback(fetchId);
	}

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseHostParentsParams(
  JSONParser &parser,
  VMInfoVect &vmInfoVect,
  JSONRPCErrorObject &errObj
)
{
	StringList errors;
	CHECK_MANDATORY_OBJECT_EXISTENCE("hostParents", errObj);
	parser.startObject("hostParents");
	size_t num = parser.countElements();
	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse vm_info contents.\n");
			return false;
		}

		VMInfo vmInfo;
		string childHostId, parentHostId;
		PARSE_AS_MANDATORY("childHostId", childHostId, errObj);
		vmInfo.hostId = StringUtils::toUint64(childHostId);
		PARSE_AS_MANDATORY("parentHostId", parentHostId, errObj);
		vmInfo.hypervisorHostId = StringUtils::toUint64(parentHostId);
		parser.endElement();

		vmInfoVect.push_back(vmInfo);
	}
	parser.endObject(); // hostsParents
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHostParents(
  JSONParser &parser)
{
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	VMInfoVect vmInfoVect;
	parser.startObject("params");

	JSONRPCErrorObject errObj;
	bool succeeded = parseHostParentsParams(parser, vmInfoVect, errObj);
	if (!succeeded) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidHostParents = parseUpdateType(parser, updateType, errObj);
	// TODO: implement validation for hostParents
	for (auto vmInfo : vmInfoVect)
		dbHost.upsertVMInfo(vmInfo);
	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_HOST_PARENT);
	}

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseArmInfoParams(JSONParser &parser, ArmInfo &armInfo,
			       JSONRPCErrorObject &errObj)
{
	string status;
	PARSE_AS_MANDATORY("lastStatus", status, errObj);
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
	PARSE_AS_MANDATORY("failureReason", armInfo.failureComment, errObj);
	timespec successTime, failureTime;
	parseTimeStamp(parser, "lastSuccessTime", successTime);
	parseTimeStamp(parser, "lastFailureTime", failureTime);
	SmartTime lastSuccessTime(successTime);
	SmartTime lastFailureTime(failureTime);
	armInfo.statUpdateTime = lastSuccessTime;
	armInfo.lastFailureTime = lastFailureTime;

	int64_t numSuccess, numFailure;
	PARSE_AS_MANDATORY("numSuccess", numSuccess, errObj);
	PARSE_AS_MANDATORY("numFailure", numFailure, errObj);
	armInfo.numUpdate = (size_t)numSuccess;
	armInfo.numFailure = (size_t)numFailure;

	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutArmInfo(
  JSONParser &parser)
{
	ArmStatus status;
	ArmInfo armInfo;
	parser.startObject("params");

	JSONRPCErrorObject errObj;
	bool succeeded = parseArmInfoParams(parser, armInfo, errObj);
	if (!succeeded) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid request object given.", &parser);
	}
	status.setArmInfo(armInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
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
