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

#ifndef FaceRestPrivate_h
#define FaceRestPrivate_h

#include "FaceRest.h"
#include <StringUtils.h>
#include <AtomicValue.h>

static const uint64_t INVALID_ID = -1;

typedef std::map<TriggerIdType, std::string> TriggerBriefMap;
typedef std::map<ServerIdType, TriggerBriefMap> TriggerBriefMaps;

typedef void (*RestHandler) (FaceRest::ResourceHandler *job);

enum FormatType {
	FORMAT_HTML,
	FORMAT_JSON,
	FORMAT_JSONP,
};

struct FaceRest::ResourceHandler
{
	mlpl::AtomicValue<size_t> refCount;

	// arguments of SoupServerCallback
	SoupMessage       *message;
	std::string        path;
	mlpl::StringVector pathElements;
	GHashTable        *query;
	SoupClientContext *client;

	FaceRest   *faceRest;
	RestHandler handler;

	// parsed data
	std::string formatString;
	FormatType  formatType;
	const char *mimeType;
	std::string jsonpCallbackName;
	std::string sessionId;
	UserIdType  userId;
	bool        replyIsPrepared;
	DataQueryContextPtr dataQueryContextPtr;

	ResourceHandler(FaceRest *_faceRest, RestHandler _handler,
			SoupMessage *_msg, const char *_path,
			GHashTable *_query, SoupClientContext *_client);
	virtual ~ResourceHandler();

	void ref(void);
	void unref(void);

	SoupServer *getSoupServer(void);
	GMainContext *getGMainContext(void);
	bool pathIsUserMe(void);
	bool prepare(void);
	void pauseResponse(void);
	void unpauseResponse(void);

	std::string getResourceName(int nest = 0);
	std::string getResourceIdString(int nest = 0);
	uint64_t    getResourceId(int nest = 0);

	void replyError(const HatoholError &hatoholError);
	void replyError(const HatoholErrorCode &errorCode,
			const std::string &optionMessage = "");
	void replyJsonData(JsonBuilderAgent &agent);
	void addServersMap(JsonBuilderAgent &agent,
			   TriggerBriefMaps *triggerMaps = NULL,
			   bool lookupTriggerBrief = false);

	static void addHatoholError(JsonBuilderAgent &agent,
	                            const HatoholError &err);

        // TODO: Move to RestResourceHost
	void itemFetchedCallback(ClosureBase *closure);

private:
	std::string getJsonpCallbackName(void);
	bool parseFormatType(void);
};

#define REPLY_ERROR(JOB, ERR_CODE, ERR_MSG_FMT, ...) \
do { \
	std::string optMsg \
	  = mlpl::StringUtils::sprintf(ERR_MSG_FMT, ##__VA_ARGS__); \
	JOB->replyError(ERR_CODE, optMsg); \
} while (0)

template<typename T>
HatoholError getParam(
  GHashTable *query, const char *paramName, const char *scanFmt, T &dest)
{
	char *value = (char *)g_hash_table_lookup(query, paramName);
	if (!value)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, paramName);

	if (sscanf(value, scanFmt, &dest) != 1) {
		std::string optMsg = mlpl::StringUtils::sprintf("%s: %s",
		                                     paramName, value);
		return HatoholError(HTERR_INVALID_PARAMETER, optMsg);
	}
	return HatoholError(HTERR_OK);
}

template<typename T>
bool getParamWithErrorReply(
  FaceRest::ResourceHandler *job, const char *paramName, const char *scanFmt,
  T &dest, bool *exist)
{
	char *value = (char *)g_hash_table_lookup(job->query, paramName);
	if (exist)
		*exist = value;
	if (!value)
		return true;

	if (sscanf(value, scanFmt, &dest) != 1) {
		REPLY_ERROR(job, HTERR_INVALID_PARAMETER,
		            "%s: %s", paramName, value);
		return false;
	}
	return true;
}

#endif // FaceRestPrivate_h
