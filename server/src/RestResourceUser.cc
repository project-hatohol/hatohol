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

#include "RestResourceUser.h"
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;

const char *RestResourceUser::pathForUser     = "/user";
const char *RestResourceUser::pathForUserRole = "/user-role";

void RestResourceUser::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForUser,
	  new RestResourceUserFactory(faceRest, handlerUser));
	faceRest->addResourceHandlerFactory(
	  pathForUserRole,
	  new RestResourceUserFactory(faceRest, handlerUserRole));
}

RestResourceUser::RestResourceUser(
  FaceRest *faceRest, RestHandler handler)
: FaceRest::ResourceHandler(faceRest, handler)
{
}

RestResourceUser::~RestResourceUser()
{
}

static HatoholError parseUserParameter(
  UserInfo &userInfo, GHashTable *query, bool allowEmpty = false)
{
	char *value;

	// name
	value = (char *)g_hash_table_lookup(query, "name");
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "name");
	if (value)
		userInfo.name = value;

	// password
	value = (char *)g_hash_table_lookup(query, "password");
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "password");
	userInfo.password = value ? value : "";

	// flags
	HatoholError err = getParam<OperationPrivilegeFlag>(
		query, "flags", "%" FMT_OPPRVLG, userInfo.flags);
	if (err != HTERR_OK) {
		if (!allowEmpty || err != HTERR_NOT_FOUND_PARAMETER)
			return err;
	}
	return HatoholError(HTERR_OK);
}

static HatoholError parseUserRoleParameter(
  UserRoleInfo &userRoleInfo, GHashTable *query, bool allowEmpty = false)
{
	char *value;

	// name
	value = (char *)g_hash_table_lookup(query, "name");
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "name");
	if (value)
		userRoleInfo.name = value;

	// flags
	HatoholError err = getParam<OperationPrivilegeFlag>(
		query, "flags", "%" FMT_OPPRVLG, userRoleInfo.flags);
	if (err != HTERR_OK && !allowEmpty)
		return err;
	return HatoholError(HTERR_OK);
}

void RestResourceUser::handlerUser(ResourceHandler *job)
{
	// handle sub-resources
	string resourceName = job->getResourceName(1);
	if (StringUtils::casecmp(resourceName, "access-info")) {
		handlerAccessInfo(job);
		return;
	}

	// handle "user" resource itself
	if (StringUtils::casecmp(job->m_message->method, "GET")) {
		handlerGetUser(job);
	} else if (StringUtils::casecmp(job->m_message->method, "POST")) {
		handlerPostUser(job);
	} else if (StringUtils::casecmp(job->m_message->method, "PUT")) {
		handlerPutUser(job);
	} else if (StringUtils::casecmp(job->m_message->method, "DELETE")) {
		handlerDeleteUser(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->m_message->method);
		soup_message_set_status(job->m_message,
		                        SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->m_replyIsPrepared = true;
	}
}

static void addUserRolesMap(
  FaceRest::ResourceHandler *job, JsonBuilderAgent &agent)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	UserRoleInfoList userRoleList;
	UserRoleQueryOption option(job->m_dataQueryContextPtr);
	dataStore->getUserRoleList(userRoleList, option);

	agent.startObject("userRoles");
	UserRoleInfoListIterator it = userRoleList.begin();
	agent.startObject(StringUtils::toString(NONE_PRIVILEGE));
	agent.add("name", "Guest");
	agent.endObject();
	agent.startObject(StringUtils::toString(ALL_PRIVILEGES));
	agent.add("name", "Admin");
	agent.endObject();
	for (; it != userRoleList.end(); ++it) {
		UserRoleInfo &userRoleInfo = *it;
		agent.startObject(StringUtils::toString(userRoleInfo.flags));
		agent.add("name", userRoleInfo.name);
		agent.endObject();
	}
	agent.endObject();
}

void RestResourceUser::handlerGetUser(ResourceHandler *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	UserInfoList userList;
	UserQueryOption option(job->m_dataQueryContextPtr);
	if (job->pathIsUserMe())
		option.queryOnlyMyself();
	dataStore->getUserList(userList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfUsers", userList.size());
	agent.startArray("users");
	UserInfoListIterator it = userList.begin();
	for (; it != userList.end(); ++it) {
		const UserInfo &userInfo = *it;
		agent.startObject();
		agent.add("userId",  userInfo.id);
		agent.add("name", userInfo.name);
		agent.add("flags", userInfo.flags);
		agent.endObject();
	}
	agent.endArray();
	addUserRolesMap(job, agent);
	agent.endObject();

	job->replyJsonData(agent);
}

void RestResourceUser::handlerPostUser(ResourceHandler *job)
{
	UserInfo userInfo;
	HatoholError err = parseUserParameter(userInfo, job->m_query);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// try to add
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->addUser(
	  userInfo, job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userInfo.id);
	agent.endObject();
	job->replyJsonData(agent);
}

void RestResourceUser::handlerPutUser(ResourceHandler *job)
{
	UserInfo userInfo;
	userInfo.id = job->getResourceId();
	if (userInfo.id == INVALID_USER_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", job->getResourceIdString().c_str());
		return;
	}

	DBClientUser dbUser;
	bool exist = dbUser.getUserInfo(userInfo, userInfo.id);
	if (!exist) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_TARGET_RECORD,
		            "id: %" FMT_USER_ID, userInfo.id);
		return;
	}
	bool allowEmpty = true;
	HatoholError err = parseUserParameter(userInfo, job->m_query,
					      allowEmpty);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// try to update
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->updateUser(
	  userInfo, job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userInfo.id);
	agent.endObject();
	job->replyJsonData(agent);
}

void RestResourceUser::handlerDeleteUser(ResourceHandler *job)
{
	uint64_t userId = job->getResourceId();
	if (userId == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", job->getResourceIdString().c_str());
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err =
	  dataStore->deleteUser(
	    userId, job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userId);
	agent.endObject();
	job->replyJsonData(agent);
}

void RestResourceUser::handlerAccessInfo(ResourceHandler *job)
{
	if (StringUtils::casecmp(job->m_message->method, "GET")) {
		handlerGetAccessInfo(job);
	} else if (StringUtils::casecmp(job->m_message->method, "POST")) {
		handlerPostAccessInfo(job);
	} else if (StringUtils::casecmp(job->m_message->method, "DELETE")) {
		handlerDeleteAccessInfo(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->m_message->method);
		soup_message_set_status(job->m_message,
		                        SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->m_replyIsPrepared = true;
	}
}

void RestResourceUser::handlerGetAccessInfo(ResourceHandler *job)
{
	AccessInfoQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetUserId(job->getResourceId());

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerAccessInfoMap serversMap;
	HatoholError error = dataStore->getAccessInfoMap(serversMap, option);
	if (error != HTERR_OK) {
		job->replyError(error);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	ServerAccessInfoMapIterator it = serversMap.begin();
	agent.startObject("allowedServers");
	for (; it != serversMap.end(); ++it) {
		const ServerIdType &serverId = it->first;
		string serverIdString;
		HostGrpAccessInfoMap *hostgroupsMap = it->second;
		if (serverId == ALL_SERVERS)
			serverIdString = StringUtils::toString(-1);
		else
			serverIdString = StringUtils::toString(serverId);
		agent.startObject(serverIdString);
		agent.startObject("allowedHostgroups");
		HostGrpAccessInfoMapIterator it2 = hostgroupsMap->begin();
		for (; it2 != hostgroupsMap->end(); it2++) {
			AccessInfo *info = it2->second;
			uint64_t hostgroupId = it2->first;
			string hostgroupIdString;
			if (hostgroupId == ALL_HOST_GROUPS) {
				hostgroupIdString = StringUtils::toString(-1);
			} else {
				hostgroupIdString
				  = StringUtils::sprintf("%" PRIu64,
				                         hostgroupId);
			}
			agent.startObject(hostgroupIdString);
			agent.add("accessInfoId", info->id);
			agent.endObject();
		}
		agent.endObject();
		agent.endObject();
	}
	agent.endObject();
	agent.endObject();

	DBClientUser::destroyServerAccessInfoMap(serversMap);

	job->replyJsonData(agent);
}

void RestResourceUser::handlerPostAccessInfo(ResourceHandler *job)
{
	// Get query parameters
	bool exist;
	bool succeeded;
	AccessInfo accessInfo;

	// userId
	int userIdPos = 0;
	accessInfo.userId = job->getResourceId(userIdPos);

	// serverId
	succeeded = getParamWithErrorReply<ServerIdType>(
	              job, "serverId", "%" FMT_SERVER_ID,
	              accessInfo.serverId, &exist);
	if (!succeeded)
		return;
	if (!exist) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "serverId");
		return;
	}

	// hostgroupId
	succeeded = getParamWithErrorReply<uint64_t>(
	              job, "hostgroupId", "%" PRIu64,
	              accessInfo.hostgroupId, &exist);
	if (!succeeded)
		return;
	if (!exist) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "hostgroupId");
		return;
	}

	// try to add
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err =
	  dataStore->addAccessInfo(
	    accessInfo, job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", accessInfo.id);
	agent.endObject();
	job->replyJsonData(agent);
}

void RestResourceUser::handlerDeleteAccessInfo(ResourceHandler *job)
{
	int nest = 1;
	uint64_t id = job->getResourceId(nest);
	if (id == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
			    "id: %s", job->getResourceIdString(nest).c_str());
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err =
	  dataStore->deleteAccessInfo(
	    id, job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", id);
	agent.endObject();
	job->replyJsonData(agent);
}

void RestResourceUser::handlerUserRole(ResourceHandler *job)
{
	if (StringUtils::casecmp(job->m_message->method, "GET")) {
		handlerGetUserRole(job);
	} else if (StringUtils::casecmp(job->m_message->method, "POST")) {
		handlerPostUserRole(job);
	} else if (StringUtils::casecmp(job->m_message->method, "PUT")) {
		handlerPutUserRole(job);
	} else if (StringUtils::casecmp(job->m_message->method, "DELETE")) {
		handlerDeleteUserRole(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->m_message->method);
		soup_message_set_status(job->m_message,
		                        SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->m_replyIsPrepared = true;
	}
}

void RestResourceUser::handlerPutUserRole(ResourceHandler *job)
{
	uint64_t id = job->getResourceId();
	if (id == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", job->getResourceIdString().c_str());
		return;
	}
	UserRoleInfo userRoleInfo;
	userRoleInfo.id = id;

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	UserRoleInfoList userRoleList;
	UserRoleQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetUserRoleId(userRoleInfo.id);
	dataStore->getUserRoleList(userRoleList, option);
	if (userRoleList.empty()) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_TARGET_RECORD,
		            "id: %" FMT_USER_ID, userRoleInfo.id);
		return;
	}
	userRoleInfo = *(userRoleList.begin());

	bool allowEmpty = true;
	HatoholError err = parseUserRoleParameter(userRoleInfo, job->m_query,
						  allowEmpty);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// try to update
	err = dataStore->updateUserRole(
	  userRoleInfo, job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userRoleInfo.id);
	agent.endObject();
	job->replyJsonData(agent);
}

void RestResourceUser::handlerGetUserRole(ResourceHandler *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	UserRoleInfoList userRoleList;
	UserRoleQueryOption option(job->m_dataQueryContextPtr);
	dataStore->getUserRoleList(userRoleList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfUserRoles", userRoleList.size());
	agent.startArray("userRoles");
	UserRoleInfoListIterator it = userRoleList.begin();
	for (; it != userRoleList.end(); ++it) {
		const UserRoleInfo &userRoleInfo = *it;
		agent.startObject();
		agent.add("userRoleId",  userRoleInfo.id);
		agent.add("name", userRoleInfo.name);
		agent.add("flags", userRoleInfo.flags);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	job->replyJsonData(agent);
}

void RestResourceUser::handlerPostUserRole(ResourceHandler *job)
{
	UserRoleInfo userRoleInfo;
	HatoholError err = parseUserRoleParameter(userRoleInfo, job->m_query);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// try to add
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->addUserRole(
	  userRoleInfo, job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userRoleInfo.id);
	agent.endObject();
	job->replyJsonData(agent);
}

void RestResourceUser::handlerDeleteUserRole(ResourceHandler *job)
{
	uint64_t id = job->getResourceId();
	if (id == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", job->getResourceIdString().c_str());
		return;
	}
	UserRoleIdType userRoleId = id;

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err =
	  dataStore->deleteUserRole(
	    userRoleId, job->m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userRoleId);
	agent.endObject();
	job->replyJsonData(agent);
}

HatoholError FaceRest::updateOrAddUser(GHashTable *query,
                                       UserQueryOption &option)
{
	UserInfo userInfo;
	HatoholError err = parseUserParameter(userInfo, query);
	if (err != HTERR_OK)
		return err;

	err = option.setTargetName(userInfo.name);
	if (err != HTERR_OK)
		return err;
	UserInfoList userList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getUserList(userList, option);

	if (userList.empty()) {
		err = dataStore->addUser(userInfo, option);
	} else {
		userInfo.id = userList.begin()->id;
		err = dataStore->updateUser(userInfo, option);
	}
	if (err != HTERR_OK)
		return err;
	return HatoholError(HTERR_OK);
}

RestResourceUserFactory::RestResourceUserFactory(
  FaceRest *faceRest, RestHandler handler)
: FaceRest::ResourceHandlerFactory(faceRest, handler)
{
}

FaceRest::ResourceHandler *RestResourceUserFactory::createHandler()
{
	return new RestResourceUser(m_faceRest, m_handler);
}
