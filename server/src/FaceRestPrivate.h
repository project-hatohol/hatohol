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

	bool httpMethodIs(const char *method);
	std::string getResourceName(int nest = 0);
	std::string getResourceIdString(int nest = 0);
	uint64_t    getResourceId(int nest = 0);

	void replyError(const HatoholError &hatoholError,
			const guint &statusCode = SOUP_STATUS_OK);
	void replyError(const HatoholErrorCode &errorCode,
			const std::string &optionMessage = "",
			const guint &statusCode = SOUP_STATUS_OK);
	void replyHttpStatus(const guint &statusCode);
	void replyJSONData(JSONBuilder &agent, const guint &statusCode = SOUP_STATUS_OK);
	void addServersMap(JSONBuilder &agent,
			   TriggerBriefMaps *triggerMaps = NULL,
			   bool lookupTriggerBrief = false);
	HatoholError addHostgroupsMap(JSONBuilder &outputJSON,
				      const MonitoringServerInfo &serverInfo,
				      HostgroupVect &hostgroups /* out */);

	static void addHatoholError(JSONBuilder &agent,
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
	std::string getJSONPCallbackName(void);
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

template <class T>
struct FaceRestResourceHandlerSimpleFactoryTemplate :
  public FaceRest::ResourceHandlerFactory
{
	FaceRestResourceHandlerSimpleFactoryTemplate(
	  FaceRest *faceRest, typename T::HandlerFunc handler)
	: FaceRest::ResourceHandlerFactory(faceRest, NULL),
	  m_handlerFunc(handler)
	{
	}

	virtual FaceRest::ResourceHandler *createHandler(void) override
	{
		return new T(m_faceRest, m_handlerFunc);
	}

	typename T::HandlerFunc m_handlerFunc;
};

#define REPLY_ERROR(JOB, ERR_CODE, ERR_MSG_FMT, ...) \
do { \
	std::string optMsg \
	  = mlpl::StringUtils::sprintf(ERR_MSG_FMT, ##__VA_ARGS__); \
	JOB->replyError(ERR_CODE, optMsg); \
} while (0)

template<typename T>
int scanParam(const char *value, const char *scanFmt, T &dest)
{
	return sscanf(value, scanFmt, &dest);
}

template<>
int scanParam(const char *value, const char *scanFmt, std::string &dest);

template<typename T>
HatoholError getParam(
  GHashTable *query, const char *paramName, const char *scanFmt, T &dest)
{
	char *value = (char *)g_hash_table_lookup(query, paramName);
	if (!value)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, paramName);

	if (scanParam<T>(value, scanFmt, dest) != 1) {
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
	HatoholError err = getParam<T>(job->m_query, paramName, scanFmt, dest);
	if (exist)
		*exist = (err != HTERR_NOT_FOUND_PARAMETER);
	if (err == HTERR_NOT_FOUND_PARAMETER)
		return true;

	if (err != HTERR_OK) {
		REPLY_ERROR(job, err.getCode(),
		            "%s", err.getOptionMessage().c_str());
		return false;
	}
	return true;
}

#endif // FaceRestPrivate_h
