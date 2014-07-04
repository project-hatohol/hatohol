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
	struct ResourceHandler;

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
	static void addServersMap(FaceRest::ResourceHandler *job,
				  JsonBuilderAgent &agent,
				  TriggerBriefMaps *triggerMaps = NULL,
				  bool lookupTriggerBrief = false);
	static void replyGetItem(ResourceHandler *job);
	static void finishRestJobIfNeeded(ResourceHandler *job);

	// handlers
	static void
	  handlerDefault(SoupServer *server, SoupMessage *msg,
	                 const char *path, GHashTable *query,
	                 SoupClientContext *client, gpointer user_data);
	static void queueRestJob
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);
	static void launchHandlerInTryBlock(ResourceHandler *job);

	static void handlerHelloPage(ResourceHandler *job);
	static void handlerTest(ResourceHandler *job);
	static void handlerLogin(ResourceHandler *job);
	static void handlerLogout(ResourceHandler *job);
	static void handlerGetOverview(ResourceHandler *job);
	static void handlerServer(ResourceHandler *job);
	static void handlerGetServer(ResourceHandler *job);
	static void handlerPostServer(ResourceHandler *job);
	static void handlerPutServer(ResourceHandler *job);
	static void handlerDeleteServer(ResourceHandler *job);
	static void handlerServerConnStat(ResourceHandler *job);
	static void handlerGetHost(ResourceHandler *job);
	static void handlerGetTrigger(ResourceHandler *job);
	static void handlerGetEvent(ResourceHandler *job);
	static void handlerGetItem(ResourceHandler *job);

	static void handlerAction(ResourceHandler *job);
	static void handlerGetAction(ResourceHandler *job);
	static void handlerPostAction(ResourceHandler *job);
	static void handlerDeleteAction(ResourceHandler *job);

	static void handlerGetHostgroup(ResourceHandler *job);

	static void handlerUser(ResourceHandler *job);
	static void handlerGetUser(ResourceHandler *job);
	static void handlerPostUser(ResourceHandler *job);
	static void handlerPutUser(ResourceHandler *job);
	static void handlerDeleteUser(ResourceHandler *job);

	static void handlerAccessInfo(ResourceHandler *job);
	static void handlerGetAccessInfo(ResourceHandler *job);
	static void handlerPostAccessInfo(ResourceHandler *job);
	static void handlerDeleteAccessInfo(ResourceHandler *job);

	static void handlerUserRole(ResourceHandler *job);
	static void handlerGetUserRole(ResourceHandler *job);
	static void handlerPostUserRole(ResourceHandler *job);
	static void handlerPutUserRole(ResourceHandler *job);
	static void handlerDeleteUserRole(ResourceHandler *job);

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
