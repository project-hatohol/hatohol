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
