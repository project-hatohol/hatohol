/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include "RestResourceServer.h"
#include "CacheServiceDBClient.h"
#include "UnifiedDataStore.h"
#include "HatoholArmPluginInterface.h"
#include "HatoholArmPluginGate.h"

using namespace std;
using namespace mlpl;

const char *RestResourceServer::pathForServer         = "/server";
const char *RestResourceServer::pathForServerConnStat = "/server-conn-stat";

void RestResourceServer::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForServer,
	  new RestResourceServerFactory(faceRest, handlerServer));
	faceRest->addResourceHandlerFactory(
	  pathForServerConnStat,
	  new RestResourceServerFactory(faceRest, handlerServerConnStat));
}

RestResourceServer::RestResourceServer(
  FaceRest *_faceRest, RestHandler _handler)
: FaceRest::ResourceHandler(_faceRest, _handler)
{
}

RestResourceServer::~RestResourceServer()
{
}

static bool canUpdateServer(
  const OperationPrivilege &privilege, const MonitoringServerInfo &serverInfo)
{
        if (privilege.has(OPPRVLG_UPDATE_ALL_SERVER))
                return true;
        if (!privilege.has(OPPRVLG_UPDATE_SERVER))
                return false;
        CacheServiceDBClient cache;
        DBClientUser *dbUser = cache.getUser();
        return dbUser->isAccessible(serverInfo.id, privilege);
}

static void addNumberOfAllowedHostgroups(UnifiedDataStore *dataStore,
                                         const UserIdType &userId,
                                         const UserIdType &targetUser,
                                         const ServerIdType &targetServer,
                                         JsonBuilderAgent &outputJson)
{
	HostgroupsQueryOption hostgroupOption(userId);
	AccessInfoQueryOption allowedHostgroupOption(userId);
	hostgroupOption.setTargetServerId(targetServer);
	allowedHostgroupOption.setTargetUserId(targetUser);
	HostgroupInfoList hostgroupInfoList;
	ServerAccessInfoMap serversMap;
	dataStore->getHostgroupInfoList(hostgroupInfoList, hostgroupOption);
	dataStore->getAccessInfoMap(serversMap, allowedHostgroupOption);

	size_t numberOfHostgroups = hostgroupInfoList.size();
	size_t numberOfAllowedHostgroups = 0;
	ServerAccessInfoMapIterator serverIt = serversMap.find(targetServer);
	if (serverIt != serversMap.end()) {
		HostGrpAccessInfoMap *hostgroupsMap = serverIt->second;
		numberOfAllowedHostgroups = hostgroupsMap->size();
		HostGrpAccessInfoMapIterator hostgroupIt
		  = hostgroupsMap->begin();
		for (; hostgroupIt != hostgroupsMap->end(); ++hostgroupIt) {
			HostgroupIdType hostgroupId = hostgroupIt->first;
			if (hostgroupId == ALL_HOST_GROUPS)
				numberOfAllowedHostgroups--;
		}
	}

	outputJson.add("numberOfHostgroups", numberOfHostgroups);
	outputJson.add("numberOfAllowedHostgroups", numberOfAllowedHostgroups);
}

static void addServers(FaceRest::ResourceHandler *job, JsonBuilderAgent &agent,
                       const ServerIdType &targetServerId,
                       const bool &showHostgroupInfo,
                       const UserIdType &targetUserId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	MonitoringServerInfoList monitoringServers;
	ArmPluginInfoVect armPluginInfoVect;
	ServerQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetServerId(targetServerId);
	dataStore->getTargetServers(monitoringServers, option,
	                            &armPluginInfoVect);

	agent.add("numberOfServers", monitoringServers.size());
	agent.startArray("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	ArmPluginInfoVectConstIterator pluginIt = armPluginInfoVect.begin();
	HATOHOL_ASSERT(monitoringServers.size() == armPluginInfoVect.size(),
	               "The nubmer of elements differs: %zd, %zd",
	               monitoringServers.size(), armPluginInfoVect.size());
	for (; it != monitoringServers.end(); ++it, ++pluginIt) {
		MonitoringServerInfo &serverInfo = *it;
		agent.startObject();
		agent.add("id", serverInfo.id);
		agent.add("type", serverInfo.type);
		agent.add("hostName", serverInfo.hostName);
		agent.add("ipAddress", serverInfo.ipAddress);
		agent.add("nickname", serverInfo.nickname);
		agent.add("port", serverInfo.port);
		agent.add("pollingInterval", serverInfo.pollingIntervalSec);
		agent.add("retryInterval", serverInfo.retryIntervalSec);
		if (canUpdateServer(option, serverInfo)) {
			// Shouldn't show account information of the server to
			// a user who isn't allowed to update it.
			agent.add("userName", serverInfo.userName);
			agent.add("password", serverInfo.password);
			agent.add("dbName", serverInfo.dbName);
		}
		if (showHostgroupInfo) {
			addNumberOfAllowedHostgroups(dataStore, job->m_userId,
			                             targetUserId, serverInfo.id,
			                             agent);
		}
		if (pluginIt->id != INVALID_ARM_PLUGIN_INFO_ID) {
			const bool passiveMode =
			  (pluginIt->path == HatoholArmPluginGate::PassivePluginQuasiPath);
			if (passiveMode)
				agent.addTrue("passiveMode");
			else
				agent.addFalse("passiveMode");
			agent.add("brokerUrl", pluginIt->brokerUrl);
			agent.add("staticQueueAddress",
			          pluginIt->staticQueueAddress);
		}
		agent.endObject();
	}
	agent.endArray();
}

void RestResourceServer::handlerServer(ResourceHandler *job)
{
	if (StringUtils::casecmp(job->m_message->method, "GET")) {
		handlerGetServer(job);
	} else if (StringUtils::casecmp(job->m_message->method, "POST")) {
		handlerPostServer(job);
	} else if (StringUtils::casecmp(job->m_message->method, "PUT")) {
		handlerPutServer(job);
	} else if (StringUtils::casecmp(job->m_message->method, "DELETE")) {
		handlerDeleteServer(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->m_message->method);
		soup_message_set_status(job->m_message,
					SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->m_replyIsPrepared = true;
	}
}

static void parseQueryServerId(GHashTable *query, ServerIdType &serverId)
{
	serverId = ALL_SERVERS;
	if (!query)
		return;
	gchar *value = (gchar *)g_hash_table_lookup(query, "serverId");
	if (!value)
		return;

	ServerIdType svId;
	if (sscanf(value, "%" FMT_SERVER_ID, &svId) == 1)
		serverId = svId;
	else
		MLPL_INFO("Invalid requested ID: %s\n", value);
}

static bool parseQueryShowHostgroupInfo(GHashTable *query, UserIdType &targetUserId)
{
	if (!query)
		return false;

	char *charUserId = (char *)g_hash_table_lookup(query, "targetUser");
	if (!charUserId)
		return false;
	sscanf(charUserId, "%" FMT_USER_ID, &targetUserId);

	int showHostgroup;
	char *value = (char *)g_hash_table_lookup(query, "showHostgroup");
	if (!value)
		return false;
	sscanf(value, "%d", &showHostgroup);
	if (showHostgroup == 1)
		return true;
	else
		return false;
}

void RestResourceServer::handlerGetServer(ResourceHandler *job)
{
	ServerIdType targetServerId;
	UserIdType targetUserId = 0;
	bool showHostgroupInfo = parseQueryShowHostgroupInfo(job->m_query,
	                                                     targetUserId);
	parseQueryServerId(job->m_query, targetServerId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addServers(job, agent, targetServerId, showHostgroupInfo, targetUserId);
	agent.endObject();

	job->replyJsonData(agent);
}

static HatoholError parseServerParameter(
  MonitoringServerInfo &svInfo, ArmPluginInfo &armPluginInfo,
  GHashTable *query, const bool &forUpdate = false)
{
	const bool allowEmpty = forUpdate;
	HatoholError err;
	char *value;

	// type
	err = getParam<MonitoringSystemType>(
		query, "type", "%d", svInfo.type);
	if (err != HTERR_OK) {
		if (!allowEmpty || err != HTERR_NOT_FOUND_PARAMETER)
			return err;
	}

	// hostname
	value = (char *)g_hash_table_lookup(query, "hostName");
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "hostName");
	svInfo.hostName = value;

	// ipAddress
	value = (char *)g_hash_table_lookup(query, "ipAddress");
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "ipAddress");
	svInfo.ipAddress = value;

	// nickname
	value = (char *)g_hash_table_lookup(query, "nickname");
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "nickname");
	svInfo.nickname = value;

	// port
	err = getParam<int>(
		query, "port", "%d", svInfo.port);
	if (err != HTERR_OK) {
		if (!allowEmpty || err != HTERR_NOT_FOUND_PARAMETER)
			return err;
	}

	// polling
	err = getParam<int>(
		query, "pollingInterval", "%d", svInfo.pollingIntervalSec);
	if (err != HTERR_OK) {
		if (!allowEmpty || err != HTERR_NOT_FOUND_PARAMETER)
			return err;
	}

	// retry
	err = getParam<int>(
		query, "retryInterval", "%d", svInfo.retryIntervalSec);
	if (err != HTERR_OK && !allowEmpty)
		return err;

	// username
	value = (char *)g_hash_table_lookup(query, "userName");
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "userName");
	svInfo.userName = value;

	// password
	value = (char *)g_hash_table_lookup(query, "password");
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "password");
	svInfo.password = value;

	// dbname
	if (svInfo.type == MONITORING_SYSTEM_NAGIOS) {
		value = (char *)g_hash_table_lookup(query, "dbName");
		if (!value && !allowEmpty)
			return HatoholError(HTERR_NOT_FOUND_PARAMETER,
					    "dbName");
		svInfo.dbName = value;
	}

	//
	// HAPI's parameters
	//
	if (!DBClientConfig::isHatoholArmPlugin(svInfo.type))
		return HTERR_OK;

	if (!forUpdate) {
		armPluginInfo.id = AUTO_INCREMENT_VALUE;
		// The proper sever ID will be set later when the DB record
		// of the concerned MonitoringServerInfo is created in
		// DBClientConfig::addTargetServer()
		armPluginInfo.serverId = INVALID_SERVER_ID;
	}
	armPluginInfo.type = svInfo.type;
	armPluginInfo.path =
	  HatoholArmPluginInterface::getDefaultPluginPath(svInfo.type) ? : "";

	// passiveMode
	// Note: We don't accept a plugin path from outside for security.
	// Instead we use a flag named passiveMode.

	// TODO: We should create a method to parse Boolean value.
	value = (char *)g_hash_table_lookup(query, "passiveMode");
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "passiveMode");
	bool passiveMode = false;
	if (value)
		passiveMode = (string(value) == "true");
	if (passiveMode) {
		armPluginInfo.path =
		  HatoholArmPluginGate::PassivePluginQuasiPath;
	}

	// brokerUrl
	value = (char *)g_hash_table_lookup(query, "brokerUrl");
	if (value)
		armPluginInfo.brokerUrl = value;

	// staticQueueAddress
	value = (char *)g_hash_table_lookup(query, "staticQueueAddress");
	if (value)
		armPluginInfo.staticQueueAddress = value;

	return HTERR_OK;
}

void RestResourceServer::handlerPostServer(ResourceHandler *job)
{
	MonitoringServerInfo svInfo;
	ArmPluginInfo        armPluginInfo;
	ArmPluginInfo::initialize(armPluginInfo);
	HatoholError err;

	err = parseServerParameter(svInfo, armPluginInfo, job->m_query);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->addTargetServer(
	  svInfo, armPluginInfo,
	  job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", svInfo.id);
	agent.endObject();
	job->replyJsonData(agent);
}

void RestResourceServer::handlerPutServer(ResourceHandler *job)
{
	uint64_t serverId;
	serverId = job->getResourceId();
	if (serverId == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", job->getResourceIdString().c_str());
		return;
	}

	// check the existing record
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	MonitoringServerInfoList serversList;
	ServerQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetServerId(serverId);
	dataStore->getTargetServers(serversList, option);
	if (serversList.empty()) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_TARGET_RECORD,
		            "id: %" PRIu64, serverId);
		return;
	}

	MonitoringServerInfo serverInfo;
	ArmPluginInfo        armPluginInfo;
	serverInfo = *serversList.begin();
	serverInfo.id = serverId;
	// TODO: Use unified data store and consider wethere the 'option'
	// for privilege is needed for getting information. We've already
	// checked it above. So it's not absolutely necessary.
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	dbConfig->getArmPluginInfo(armPluginInfo, serverId);

	// check the request
	bool allowEmpty = true;
	HatoholError err = parseServerParameter(serverInfo, armPluginInfo,
	                                        job->m_query, allowEmpty);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// try to update
	err = dataStore->updateTargetServer(serverInfo, armPluginInfo, option);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", serverInfo.id);
	agent.endObject();
	job->replyJsonData(agent);
}

void RestResourceServer::handlerDeleteServer(ResourceHandler *job)
{
	uint64_t serverId = job->getResourceId();
	if (serverId == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
			    "id: %s", job->getResourceIdString().c_str());
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err =
	  dataStore->deleteTargetServer(
	    serverId, job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", serverId);
	agent.endObject();

	job->replyJsonData(agent);
}

void RestResourceServer::handlerServerConnStat(ResourceHandler *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerConnStatusVector serverConnStatVec;
	dataStore->getServerConnStatusVector(serverConnStatVec,
	                                     job->m_dataQueryContextPtr);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.startObject("serverConnStat");
	for (size_t idx = 0; idx < serverConnStatVec.size(); idx++) {
		const ServerConnStatus &svConnStat = serverConnStatVec[idx];
		const ArmInfo &armInfo = svConnStat.armInfo;
		agent.startObject(StringUtils::toString(svConnStat.serverId));
		agent.add("running",         armInfo.running);
		agent.add("status",          armInfo.stat);
		agent.add("statUpdateTime",  armInfo.statUpdateTime);
		agent.add("lastSuccessTime", armInfo.lastSuccessTime);
		agent.add("lastFailureTime", armInfo.lastFailureTime);
		agent.add("failureComment",  armInfo.failureComment);
		agent.add("numUpdate",       armInfo.numUpdate);
		agent.add("numFailure",      armInfo.numFailure);
		agent.endObject(); // serverId
	}
	agent.endObject(); // serverConnStat
	agent.endObject();

	job->replyJsonData(agent);
}

RestResourceServerFactory::RestResourceServerFactory(
  FaceRest *faceRest, RestHandler handler)
: FaceRest::ResourceHandlerFactory(faceRest, handler)
{
}

FaceRest::ResourceHandler *RestResourceServerFactory::createHandler()
{
	return new RestResourceServer(m_faceRest, m_handler);
}
