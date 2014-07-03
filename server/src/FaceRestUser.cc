#include "FaceRest.h"
#include "FaceRestPrivate.h"
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;

void FaceRest::handlerUser(RestJob *job)
{
	// handle sub-resources
	string resourceName = job->getResourceName(1);
	if (StringUtils::casecmp(resourceName, "access-info")) {
		handlerAccessInfo(job);
		return;
	}

	// handle "user" resource itself
	if (StringUtils::casecmp(job->message->method, "GET")) {
		handlerGetUser(job);
	} else if (StringUtils::casecmp(job->message->method, "POST")) {
		handlerPostUser(job);
	} else if (StringUtils::casecmp(job->message->method, "PUT")) {
		handlerPutUser(job);
	} else if (StringUtils::casecmp(job->message->method, "DELETE")) {
		handlerDeleteUser(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
		                        SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->replyIsPrepared = true;
	}
}

static void addUserRolesMap(
  FaceRest::RestJob *job, JsonBuilderAgent &agent)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	UserRoleInfoList userRoleList;
	UserRoleQueryOption option(job->dataQueryContextPtr);
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

void FaceRest::handlerGetUser(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	UserInfoList userList;
	UserQueryOption option(job->dataQueryContextPtr);
	if (job->path == PrivateContext::pathForUserMe)
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

	replyJsonData(agent, job);
}

void FaceRest::handlerPostUser(RestJob *job)
{
	UserInfo userInfo;
	HatoholError err = parseUserParameter(userInfo, job->query);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// try to add
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->addUser(
	  userInfo, job->dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerPutUser(RestJob *job)
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
	HatoholError err = parseUserParameter(userInfo, job->query,
					      allowEmpty);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// try to update
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->updateUser(
	  userInfo, job->dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerDeleteUser(RestJob *job)
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
	    userId, job->dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userId);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerAccessInfo(RestJob *job)
{
	if (StringUtils::casecmp(job->message->method, "GET")) {
		handlerGetAccessInfo(job);
	} else if (StringUtils::casecmp(job->message->method, "POST")) {
		handlerPostAccessInfo(job);
	} else if (StringUtils::casecmp(job->message->method, "DELETE")) {
		handlerDeleteAccessInfo(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
		                        SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->replyIsPrepared = true;
	}
}

void FaceRest::handlerGetAccessInfo(RestJob *job)
{
	AccessInfoQueryOption option(job->dataQueryContextPtr);
	option.setTargetUserId(job->getResourceId());

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerAccessInfoMap serversMap;
	HatoholError error = dataStore->getAccessInfoMap(serversMap, option);
	if (error != HTERR_OK) {
		replyError(job, error);
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

	replyJsonData(agent, job);
}

void FaceRest::handlerPostAccessInfo(RestJob *job)
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
	    accessInfo, job->dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", accessInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerDeleteAccessInfo(RestJob *job)
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
	    id, job->dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerUserRole(RestJob *job)
{
	if (StringUtils::casecmp(job->message->method, "GET")) {
		handlerGetUserRole(job);
	} else if (StringUtils::casecmp(job->message->method, "POST")) {
		handlerPostUserRole(job);
	} else if (StringUtils::casecmp(job->message->method, "PUT")) {
		handlerPutUserRole(job);
	} else if (StringUtils::casecmp(job->message->method, "DELETE")) {
		handlerDeleteUserRole(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
		                        SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->replyIsPrepared = true;
	}
}

void FaceRest::handlerPutUserRole(RestJob *job)
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
	UserRoleQueryOption option(job->dataQueryContextPtr);
	option.setTargetUserRoleId(userRoleInfo.id);
	dataStore->getUserRoleList(userRoleList, option);
	if (userRoleList.empty()) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_TARGET_RECORD,
		            "id: %" FMT_USER_ID, userRoleInfo.id);
		return;
	}
	userRoleInfo = *(userRoleList.begin());

	bool allowEmpty = true;
	HatoholError err = parseUserRoleParameter(userRoleInfo, job->query,
						  allowEmpty);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// try to update
	err = dataStore->updateUserRole(
	  userRoleInfo, job->dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userRoleInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerGetUserRole(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	UserRoleInfoList userRoleList;
	UserRoleQueryOption option(job->dataQueryContextPtr);
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

	replyJsonData(agent, job);
}

void FaceRest::handlerPostUserRole(RestJob *job)
{
	UserRoleInfo userRoleInfo;
	HatoholError err = parseUserRoleParameter(userRoleInfo, job->query);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// try to add
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->addUserRole(
	  userRoleInfo, job->dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userRoleInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerDeleteUserRole(RestJob *job)
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
	    userRoleId, job->dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", userRoleId);
	agent.endObject();
	replyJsonData(agent, job);
}
