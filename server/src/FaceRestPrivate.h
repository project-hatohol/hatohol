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
#include <UsedCountable.h>

static const uint64_t INVALID_ID = -1;

typedef std::map<TriggerIdType, std::string> TriggerBriefMap;
typedef std::map<ServerIdType, TriggerBriefMap> TriggerBriefMaps;

typedef void (*RestHandlerFunc) (FaceRest::ResourceHandler *job);

enum FormatType {
	FORMAT_HTML,
	FORMAT_JSON,
	FORMAT_JSONP,
};

struct FaceRest::ResourceHandler : public UsedCountable
{
public:
	ResourceHandler(FaceRest *faceRest, RestHandlerFunc handler);
	virtual ~ResourceHandler();
	virtual bool setRequest(SoupMessage *msg,
				const char *path,
				GHashTable *query,
				SoupClientContext *client);

	virtual void handle(void);
	void handleInTryBlock(void);

	SoupServer *getSoupServer(void);
	GMainContext *getGMainContext(void);
	void pauseResponse(void);
	bool unpauseResponse(bool force = false);

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
	HatoholError addHostgroupsMap(JsonBuilderAgent &outputJson,
				      const MonitoringServerInfo &serverInfo,
				      HostgroupInfoList &hostgroupList /* out */);

	static void addHatoholError(JsonBuilderAgent &agent,
	                            const HatoholError &err);

public:
	FaceRest          *m_faceRest;
	RestHandlerFunc    m_staticHandlerFunc;

	// arguments of SoupServerCallback
	SoupMessage       *m_message;
	std::string        m_path;
	mlpl::StringVector m_pathElements;
	GHashTable        *m_query;
	SoupClientContext *m_client;

	// parsed data
	std::string m_formatString;
	FormatType  m_formatType;
	const char *m_mimeType;
	std::string m_jsonpCallbackName;
	std::string m_sessionId;
	UserIdType  m_userId;
	bool        m_replyIsPrepared;
	DataQueryContextPtr m_dataQueryContextPtr;

protected:
	bool parseRequest(void);
	std::string getJsonpCallbackName(void);
	bool parseFormatType(void);
};

struct FaceRest::ResourceHandlerFactory
{
	ResourceHandlerFactory(FaceRest *faceRest, RestHandlerFunc handler);
	virtual ~ResourceHandlerFactory();
	virtual ResourceHandler *createHandler(void);
	static void destroy(gpointer data);

	FaceRest        *m_faceRest;
	RestHandlerFunc  m_staticHandlerFunc;
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
	char *value = (char *)g_hash_table_lookup(job->m_query, paramName);
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
