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
#include "MutexLock.h"
#include "AtomicValue.h"
#include <semaphore.h>
#include <errno.h>
#include <string.h>

#define REPLY_ERROR(ARG, ERR_CODE, ERR_MSG_FMT, ...) \
do { \
	string optMsg = StringUtils::sprintf(ERR_MSG_FMT, ##__VA_ARGS__); \
	replyError(ARG, ERR_CODE, optMsg); \
} while (0)

#define RETURN_IF_NOT_TEST_MODE(ARG) \
do { \
	if (!isTestMode()) { \
		replyError(ARG, HTERR_NOT_TEST_MODE); \
		return; \
	}\
} while(0)

static const uint64_t INVALID_ID = -1;

typedef void (*RestHandler) (FaceRest::RestJob *job);

struct FaceRest::PrivateContext {
	struct MainThreadCleaner;
	static bool         testMode;
	static mlpl::MutexLock lock;
	static const std::string pathForUserMe;
	guint               port;
	SoupServer         *soupServer;
	GMainContext       *gMainCtx;
	FaceRestParam      *param;
	mlpl::AtomicValue<bool>   quitRequest;

	// for async mode
	bool                  asyncMode;
	size_t                numPreLoadWorkers;
	std::set<Worker *>    workers;
	std::queue<RestJob *> restJobQueue;
	mlpl::MutexLock       restJobLock;
	sem_t                 waitJobSemaphore;

	PrivateContext(FaceRestParam *_param);
	virtual ~PrivateContext();

	static std::string initPathForUserMe(void)
	{
		return std::string(FaceRest::pathForUser) + "/me";
	}

	void pushJob(RestJob *job)
	{
		restJobLock.lock();
		restJobQueue.push(job);
		if (sem_post(&waitJobSemaphore) == -1)
			MLPL_ERR("Failed to call sem_post: %d\n",
				 errno);
		restJobLock.unlock();
	}

	bool waitJob(void)
	{
		if (sem_wait(&waitJobSemaphore) == -1)
			MLPL_ERR("Failed to call sem_wait: %d\n", errno);
		return !quitRequest.get();
	}

	RestJob *popJob(void)
	{
		RestJob *job = NULL;
		restJobLock.lock();
		if (!restJobQueue.empty()) {
			job = restJobQueue.front();
			restJobQueue.pop();
		}
		restJobLock.unlock();
		return job;
	}

	static bool isTestPath(const std::string &path)
	{
		size_t len = strlen(pathForTest);
		return (strncmp(path.c_str(), pathForTest, len) == 0);
	}
};

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

	SoupServer *server(void)
	{
		return faceRest ? faceRest->m_ctx->soupServer : NULL;
	}

	GMainContext *gMainContext(void)
	{
		return faceRest ? faceRest->m_ctx->gMainCtx : NULL;
	}

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

#endif // FaceRestPrivate_h
