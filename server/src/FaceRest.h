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

#ifndef FaceRest_h
#define FaceRest_h

#include <libsoup/soup.h>
#include "FaceBase.h"
#include "JsonBuilderAgent.h"
#include "SmartTime.h"
#include "Params.h"
#include "HatoholError.h"
#include "DBClientConfig.h"
#include "DBClientUser.h"
#include "DBClientHatohol.h"
#include "Closure.h"
#include "Utils.h"

struct FaceRestParam {
	virtual void setupDoneNotifyFunc(void)
	{
	}
};

typedef std::map<TriggerIdType, std::string> TriggerBriefMap;
typedef std::map<ServerIdType, TriggerBriefMap> TriggerBriefMaps;

class FaceRest : public FaceBase {
public:

	static int API_VERSION;
	static const char *SESSION_ID_HEADER_NAME;
	static const int DEFAULT_NUM_WORKERS;

	static void init(void);
	static void reset(const CommandLineArg &arg);
	static bool isTestMode(void);

	FaceRest(CommandLineArg &cmdArg, FaceRestParam *param = NULL);
	virtual ~FaceRest();
	virtual void waitExit(void) override;
	virtual void setNumberOfPreLoadWorkers(size_t num);

	class Worker;
	struct RestJob;

protected:
	// virtual methods
	gpointer mainThread(HatoholThreadArg *arg);

	// for async mode
	bool isAsyncMode(void);
	void startWorkers(void);
	void stopWorkers(void);

	// generic sub routines
	SoupServer   *getSoupServer(void);
	GMainContext *getGMainContext(void);
	const std::string &getPathForUserMe(void);
	size_t parseCmdArgPort(CommandLineArg &cmdArg, size_t idx);
	static void addHatoholError(JsonBuilderAgent &agent,
	                            const HatoholError &err);
	static void addServersMap(FaceRest::RestJob *job,
				  JsonBuilderAgent &agent,
				  TriggerBriefMaps *triggerMaps = NULL,
				  bool lookupTriggerBrief = false);
	static void addHostsIsMemberOfGroup(RestJob *job,
	                                   JsonBuilderAgent &agent,
	                                   uint64_t targetServerId,
	                                   uint64_t targetGroupId);
	static void replyGetItem(RestJob *job);
	static void finishRestJobIfNeeded(RestJob *job);

	// handlers
	static void
	  handlerDefault(SoupServer *server, SoupMessage *msg,
	                 const char *path, GHashTable *query,
	                 SoupClientContext *client, gpointer user_data);
	static void queueRestJob
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);
	static void launchHandlerInTryBlock(RestJob *job);

	static void handlerHelloPage(RestJob *job);
	static void handlerTest(RestJob *job);
	static void handlerLogin(RestJob *job);
	static void handlerLogout(RestJob *job);
	static void handlerGetOverview(RestJob *job);
	static void handlerServer(RestJob *job);
	static void handlerGetServer(RestJob *job);
	static void handlerPostServer(RestJob *job);
	static void handlerPutServer(RestJob *job);
	static void handlerDeleteServer(RestJob *job);
	static void handlerServerConnStat(RestJob *job);
	static void handlerGetHost(RestJob *job);
	static void handlerGetTrigger(RestJob *job);
	static void handlerGetEvent(RestJob *job);
	static void handlerGetItem(RestJob *job);

	static void handlerAction(RestJob *job);
	static void handlerGetAction(RestJob *job);
	static void handlerPostAction(RestJob *job);
	static void handlerDeleteAction(RestJob *job);

	static void handlerGetHostgroup(RestJob *job);

	static void handlerUser(RestJob *job);
	static void handlerGetUser(RestJob *job);
	static void handlerPostUser(RestJob *job);
	static void handlerPutUser(RestJob *job);
	static void handlerDeleteUser(RestJob *job);

	static void handlerAccessInfo(RestJob *job);
	static void handlerGetAccessInfo(RestJob *job);
	static void handlerPostAccessInfo(RestJob *job);
	static void handlerDeleteAccessInfo(RestJob *job);

	static void handlerUserRole(RestJob *job);
	static void handlerGetUserRole(RestJob *job);
	static void handlerPostUserRole(RestJob *job);
	static void handlerPutUserRole(RestJob *job);
	static void handlerDeleteUserRole(RestJob *job);

	void itemFetchedCallback(ClosureBase *closure);

	/**
	 * Update the user informformation if 'name' specifined in 'query'
	 * exits in the DB. Otherwise, the user is newly added.
	 * NOTE: This method is currently used for test purpose.
	 *
	 * @param query
	 * A hash table that has query parameters in the URL.
	 *
	 * @param option
	 * A UserQueryOption used for the query.
	 *
	 * @return A HatoholError is returned.
	 */
	static HatoholError updateOrAddUser(GHashTable *query,
	                                    UserQueryOption &option);

	static HatoholError parseEventParameter(EventsQueryOption &option,
						GHashTable *query);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

	static const char *pathForTest;
	static const char *pathForLogin;
	static const char *pathForLogout;
	static const char *pathForGetOverview;
	static const char *pathForServer;
	static const char *pathForServerConnStat;
	static const char *pathForGetHost;
	static const char *pathForGetTrigger;
	static const char *pathForGetEvent;
	static const char *pathForGetItem;
	static const char *pathForAction;
	static const char *pathForUser;
	static const char *pathForHostgroup;
	static const char *pathForUserRole;
};

#endif // FaceRest_h
