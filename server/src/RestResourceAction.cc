/*
 * Copyright (C) 2013-2015 Project Hatohol
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

#include "RestResourceAction.h"
#include "DBTablesAction.h"
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;

typedef FaceRestResourceHandlerArg0FactoryTemplate<RestResourceAction>
  RestResourceActionFactory;

const char *RestResourceAction::pathForAction = "/action";

void RestResourceAction::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForAction, new RestResourceActionFactory(faceRest));
}

RestResourceAction::RestResourceAction(FaceRest *faceRest)
: FaceRest::ResourceHandler(faceRest, NULL)
{
}

RestResourceAction::~RestResourceAction()
{
}

template <typename T>
static void setActionCondition(
  JSONBuilder &agent, const ActionCondition &cond,
  const string &member, ActionConditionEnableFlag bit,
  T value)
{
	if (cond.isEnable(bit))
		agent.add(member, value);
	else
		agent.addNull(member);
}

void RestResourceAction::handle(void)
{
	if (httpMethodIs("GET")) {
		handleGet();
	} else if (httpMethodIs("POST")) {
		handlePost();
	} else if (httpMethodIs("DELETE")) {
		handleDelete();
	} else if (httpMethodIs("PUT")) {
		handlePut();
	} else {
		MLPL_ERR("Unknown method: %s\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

static HatoholError parseQueryForGet(ActionsQueryOption &option,
				     GHashTable *query)
{
	ActionType actionType = option.getActionType();
	HatoholError err = getParam<ActionType>(query, "type", "%d",
						actionType);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setActionType(actionType);

	return HTERR_OK;
}

void RestResourceAction::handleGet(void)
{
	ActionsQueryOption option(m_dataQueryContextPtr);
	HatoholError err = parseQueryForGet(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ActionDefList actionList;
	err = dataStore->getActionList(actionList, option);

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err != HTERR_OK) {
		agent.endObject();
		replyJSONData(agent);
		return;
	}
	agent.add("numberOfActions", actionList.size());
	agent.startArray("actions");
	TriggerBriefMaps triggerMaps;
	ActionDefListIterator it = actionList.begin();
	for (; it != actionList.end(); ++it) {
		const ActionDef &actionDef = *it;
		const ActionCondition &cond = actionDef.condition;
		agent.startObject();
		agent.add("actionId",  actionDef.id);
		agent.add("enableBits", cond.enableBits);
		setActionCondition<ServerIdType>(
		  agent, cond, "serverId", ACTCOND_SERVER_ID, cond.serverId);
		setActionCondition<std::string>(
		   agent, cond, "hostId", ACTCOND_HOST_ID, cond.hostIdInServer);
		setActionCondition<HostgroupIdType>(
		  agent, cond, "hostgroupId", ACTCOND_HOST_GROUP_ID,
		   cond.hostgroupId);
		setActionCondition<std::string>(
		  agent, cond, "triggerId", ACTCOND_TRIGGER_ID, cond.triggerId);
		setActionCondition<uint32_t>(
		  agent, cond, "triggerStatus", ACTCOND_TRIGGER_STATUS,
		  cond.triggerStatus);
		setActionCondition<uint32_t>(
		  agent, cond, "triggerSeverity", ACTCOND_TRIGGER_SEVERITY,
		  cond.triggerSeverity);
		agent.add("triggerSeverityComparatorType",
		          cond.triggerSeverityCompType);
		agent.add("type",      actionDef.type);
		agent.add("workingDirectory",
		          actionDef.workingDir.c_str());
		agent.add("command", actionDef.command);
		agent.add("timeout", actionDef.timeout);
		agent.add("ownerUserId", actionDef.ownerUserId);
		agent.endObject();
		if (cond.isEnable(ACTCOND_TRIGGER_ID))
			triggerMaps[cond.serverId][cond.triggerId] = "";
	}
	agent.endArray();
	const bool lookupTriggerBrief = true;
	addServersMap(agent, &triggerMaps, lookupTriggerBrief);
	agent.endObject();

	replyJSONData(agent);
}

static HatoholError parseActionParameter(FaceRest::ResourceHandler *job,
                                         ActionDef &actionDef)
{
	//
	// mandatory parameters
	//
	char *value;
	bool exist;
	bool succeeded;

	// action type
	succeeded = getParamWithErrorReply<int>(
	              job, "type", "%d", (int &)actionDef.type, &exist);
	if (!succeeded)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "type");
	if (!exist) {
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "type");
	}
	if (!(actionDef.type == ACTION_COMMAND ||
		  actionDef.type == ACTION_RESIDENT ||
		  actionDef.type == ACTION_INCIDENT_SENDER)) {
		return HatoholError(HTERR_INVALID_PARAMETER,
		                    StringUtils::sprintf("type: %d", actionDef.type));
	}

	// command
	value = (char *)g_hash_table_lookup(job->m_query, "command");
	if (!value) {
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "command");
	}
	actionDef.command = value;

	//
	// optional parameters
	//
	ActionCondition &cond = actionDef.condition;

	// workingDirectory
	value = (char *)g_hash_table_lookup(job->m_query, "workingDirectory");
	if (value) {
		actionDef.workingDir = value;
	}

	// timeout
	succeeded = getParamWithErrorReply<int>(
	              job, "timeout", "%d", actionDef.timeout, &exist);
	if (!succeeded)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "timeout");
	if (!exist)
		actionDef.timeout = 0;

	// ownerUserId
	succeeded = getParamWithErrorReply<int>(
	              job, "ownerUserId", "%d", actionDef.ownerUserId, &exist);
	if (!succeeded)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "ownerUserId");
	if (!exist)
		actionDef.ownerUserId = job->m_userId;

	// serverId
	succeeded = getParamWithErrorReply<int>(
	              job, "serverId", "%d", cond.serverId, &exist);
	if (!succeeded)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "serverId");
	if (exist)
		cond.enable(ACTCOND_SERVER_ID);

	// hostId
	value = (char *)g_hash_table_lookup(job->m_query, "hostId");
	if (value) {
		cond.hostIdInServer = value;
		cond.enable(ACTCOND_HOST_ID);
	};

	// hostgroupId
	succeeded = getParamWithErrorReply<HostgroupIdType>(
	              job, "hostgroupId", "%" FMT_HOST_GROUP_ID,
	              cond.hostgroupId, &exist);
	if (!succeeded)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "hostgroupId");
	if (exist)
		cond.enable(ACTCOND_HOST_GROUP_ID);

	// triggerId
	succeeded = getParamWithErrorReply<TriggerIdType>(
	              job, "triggerId", "%" FMT_TRIGGER_ID,
	              cond.triggerId, &exist);
	if (!succeeded)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "triggerId");
	if (exist)
		cond.enable(ACTCOND_TRIGGER_ID);

	// triggerStatus
	succeeded = getParamWithErrorReply<int>(
	              job, "triggerStatus", "%d", cond.triggerStatus, &exist);
	if (!succeeded)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "triggerStatus");
	if (exist)
		cond.enable(ACTCOND_TRIGGER_STATUS);

	// triggerSeverity
	succeeded = getParamWithErrorReply<int>(
	              job, "triggerSeverity", "%d", cond.triggerSeverity, &exist);
	if (!succeeded)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "triggerSeverity");
	if (exist) {
		cond.enable(ACTCOND_TRIGGER_SEVERITY);

		// triggerSeverityComparatorType
		succeeded = getParamWithErrorReply<int>(
		              job, "triggerSeverityCompType", "%d",
		              (int &)cond.triggerSeverityCompType, &exist);
		if (!succeeded)
			return HatoholError(HTERR_NOT_FOUND_PARAMETER,
			                    "triggerSeverityCompType");
		if (!exist) {
			return HatoholError(HTERR_NOT_FOUND_PARAMETER,
			                    "triggerSeverityCompType");
		}
		if (!(cond.triggerSeverityCompType == CMP_EQ ||
		      cond.triggerSeverityCompType == CMP_EQ_GT)) {
			string message =
			  StringUtils::sprintf(
			    "type: %d", cond.triggerSeverityCompType);
			return HatoholError(HTERR_INVALID_PARAMETER, message);
		}
	}

	return HatoholError(HTERR_OK);
}

void RestResourceAction::handlePost(void)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	ActionDef actionDef;
	HatoholError err;
	err = parseActionParameter(this, actionDef);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// save the obtained action
	err = dataStore->addAction(
	    actionDef, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// make a response
	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", actionDef.id);
	agent.endObject();
	replyJSONData(agent);
}

void RestResourceAction::handleDelete(void)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	uint64_t actionId = getResourceId();
	if (actionId == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
			    "id: %s", getResourceIdString().c_str());
		return;
	}
	ActionIdList actionIdList;
	actionIdList.push_back(actionId);
	HatoholError err =
	  dataStore->deleteActionList(
	    actionIdList, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// replay
	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", actionId);
	agent.endObject();
	replyJSONData(agent);
}

void RestResourceAction::handlePut(void)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	ActionDef actionDef;
	uint64_t actionId;

	// action id
	actionId = getResourceId();
	if (actionId == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
			    "id: %s", getResourceIdString().c_str());
		return;
	}
	actionDef.id = actionId;

	HatoholError err;
	err = parseActionParameter(this, actionDef);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// save the obtained action
	err = dataStore->updateAction(
	    actionDef, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// make a response for update
	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", actionDef.id);
	agent.endObject();
	replyJSONData(agent);
}
