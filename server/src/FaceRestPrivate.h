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
#include "StringUtils.h"

static const uint64_t INVALID_ID = -1;

typedef void (*RestHandler) (FaceRest::RestJob *job);

enum FormatType {
	FORMAT_HTML,
	FORMAT_JSON,
	FORMAT_JSONP,
};

struct FaceRest::RestJob
{
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

	RestJob(FaceRest *_faceRest, RestHandler _handler,
		SoupMessage *_msg, const char *_path,
		GHashTable *_query, SoupClientContext *_client);
	virtual ~RestJob();

	SoupServer *getSoupServer(void);
	GMainContext *getGMainContext(void);
	bool pathIsUserMe(void);
	bool prepare(void);
	void pauseResponse(void);
	void unpauseResponse(void);

	std::string getResourceName(int nest = 0);
	std::string getResourceIdString(int nest = 0);
	uint64_t    getResourceId(int nest = 0);

private:
	std::string getJsonpCallbackName(void);
	bool parseFormatType(void);
};

template<typename T>
HatoholError FaceRest::getParam(
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

#define REPLY_ERROR(ARG, ERR_CODE, ERR_MSG_FMT, ...) \
do { \
	std::string optMsg \
	  = mlpl::StringUtils::sprintf(ERR_MSG_FMT, ##__VA_ARGS__); \
	replyError(ARG, ERR_CODE, optMsg); \
} while (0)

#define RETURN_IF_NOT_TEST_MODE(ARG) \
do { \
	if (!isTestMode()) { \
		replyError(ARG, HTERR_NOT_TEST_MODE); \
		return; \
	}\
} while(0)

template<typename T>
bool FaceRest::getParamWithErrorReply(
  RestJob *job, const char *paramName, const char *scanFmt,
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
