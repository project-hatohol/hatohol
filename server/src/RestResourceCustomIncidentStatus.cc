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

#include <SeparatorInjector.h>
#include "RestResourceCustomIncidentStatus.h"
#include "DBTablesConfig.h"
#include "UnifiedDataStore.h"
#include "ThreadLocalDBCache.h"

using namespace std;
using namespace mlpl;

struct RESTAPIError {
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

	bool hasErrors(void) {
		return !errors.empty();
	}

	const StringList &getErrors(void) {
		return errors;
	}
};

typedef FaceRestResourceHandlerArg0FactoryTemplate<RestResourceCustomIncidentStatus>
  RestResourceCustomIncidentStatusFactory;

const char *RestResourceCustomIncidentStatus::pathForCustomIncidentStatus =
  "/custom-incident-status";

void RestResourceCustomIncidentStatus::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForCustomIncidentStatus,
	  new RestResourceCustomIncidentStatusFactory(faceRest));
}

RestResourceCustomIncidentStatus::RestResourceCustomIncidentStatus(
  FaceRest *faceRest)
: FaceRest::ResourceHandler(faceRest)
{
}

RestResourceCustomIncidentStatus::~RestResourceCustomIncidentStatus()
{
}

void RestResourceCustomIncidentStatus::handle(void)
{
	if (httpMethodIs("GET")) {
		handleGet();
	} else if (httpMethodIs("POST")) {
		handlePost();
	} else if (httpMethodIs("PUT")) {
		handlePut();
	} else {
		MLPL_ERR("Unknown method: %s\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

void RestResourceCustomIncidentStatus::handleGet(void)
{
	CustomIncidentStatusesQueryOption option(m_dataQueryContextPtr);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	std::vector<CustomIncidentStatus> customIncidentStatuses;
	dataStore->getCustomIncidentStatuses(customIncidentStatuses, option);

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, HTERR_OK);

	agent.startArray("CustomIncidentStatuses");
	for (auto &customIncidentStatus : customIncidentStatuses) {
		agent.startObject();
		agent.add("id", customIncidentStatus.id);
		agent.add("code", customIncidentStatus.code);
		agent.add("label", customIncidentStatus.label);

		agent.endObject();
	}
	agent.endArray();

	agent.endObject();

	replyJSONData(agent);
}

#define PARSE_STRING_VALUE(STRUCT,PROPERTY,ALLOW_EMPTY, ERROBJ)		\
{									      \
	char *value = (char *)g_hash_table_lookup(query, #PROPERTY);	      \
	if (!value && !ALLOW_EMPTY)					      \
		ERROBJ.addError("'%s' is required.", #PROPERTY);	      \
	if (value)							      \
		STRUCT.PROPERTY = value;				      \
}

static HatoholError parseCustomIncidentStatusParameter(
  CustomIncidentStatus &customIncidentStatus, GHashTable *query,
  const bool &forUpdate = false)
{
	const bool allowEmpty = forUpdate;

	RESTAPIError errObj;
	PARSE_STRING_VALUE(customIncidentStatus, code, allowEmpty, errObj);
	PARSE_STRING_VALUE(customIncidentStatus, label, allowEmpty, errObj);

	if (errObj.hasErrors()) {
		SeparatorInjector spaceInjector(" ");
		string errorMessage;
		for (auto msg : errObj.getErrors()) {
			spaceInjector(errorMessage);
			errorMessage += msg;
		}
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, errorMessage);
	}

	return HatoholError(HTERR_OK);
}

void RestResourceCustomIncidentStatus::handlePost(void)
{
	CustomIncidentStatus customIncidentStatus;
	CustomIncidentStatus::initialize(customIncidentStatus);
	customIncidentStatus.id = AUTO_INCREMENT_VALUE;
	HatoholError err = parseCustomIncidentStatusParameter(customIncidentStatus,
							      m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->upsertCustomIncidentStatus(
	  customIncidentStatus, m_dataQueryContextPtr->getOperationPrivilege());

	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", customIncidentStatus.id);
	agent.endObject();
	replyJSONData(agent);
}

void RestResourceCustomIncidentStatus::handlePut(void)
{
	uint64_t customIncidentStatusId = getResourceId();
	if (customIncidentStatusId == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", getResourceIdString().c_str());
		return;
	}

	CustomIncidentStatus customIncidentStatus;
	CustomIncidentStatus::initialize(customIncidentStatus);
	customIncidentStatus.id = customIncidentStatusId;

	std::vector<CustomIncidentStatus> customIncidentStatuses;
	CustomIncidentStatusesQueryOption option(m_dataQueryContextPtr);
	option.setTargetIdList({customIncidentStatus.id});
	ThreadLocalDBCache cache;
	cache.getConfig().getCustomIncidentStatuses(customIncidentStatuses, option);
	if (customIncidentStatuses.empty()) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_TARGET_RECORD,
		            "id: %" FMT_INCIDENT_TRACKER_ID,
			    customIncidentStatus.id);
		return;
	}
	customIncidentStatus = *customIncidentStatuses.begin();

	bool allowEmpty = false;
	HatoholError err =
		parseCustomIncidentStatusParameter(customIncidentStatus,
						   m_query,
						   allowEmpty);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// try to update
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->updateCustomIncidentStatus(
	  customIncidentStatus, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// make a response
	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", customIncidentStatus.id);
	agent.endObject();
	replyJSONData(agent);
}
