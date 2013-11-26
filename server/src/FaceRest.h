/*
 * Copyright (C) 2013 Project Hatohol
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
#include "DBClientUser.h"
#include "DBClientHatohol.h"
#include "Closure.h"

struct SessionInfo {
	UserIdType userId;
	mlpl::SmartTime loginTime;
	mlpl::SmartTime lastAccessTime;

	// constructor
	SessionInfo(void);
};

struct FaceRestParam {
	virtual void setupDoneNotifyFunc(void)
	{
	}
};

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
	virtual void stop(void);

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
	size_t parseCmdArgPort(CommandLineArg &cmdArg, size_t idx);
	static void addHatoholError(JsonBuilderAgent &agent,
	                            const HatoholError &err);
	static void replyError(RestJob *job,
	                       const HatoholError &hatoholError);
	static void replyError(RestJob *job,
	                       const HatoholErrorCode &errorCode,
	                       const string &optionMessage = "");
	static string wrapForJsonp(const string &jsonBody,
                                   const string &callbackName);
	static void replyJsonData(JsonBuilderAgent &agent, RestJob *job);
	static void replyGetItem(RestJob *job);

	/**
	 * Parse 'serverId' query parameter if it exists.
	 *
	 * @param query
	 * A hash table of query parameters.
	 *
	 * @param serverId.
	 * If 'serverId' query parameter is found, the value is set to
	 * this variable. Otherwise, ALL_SERVERS is set.
	 */
	static void parseQueryServerId(GHashTable *query, uint32_t &serverId);

	/**
	 * Parse 'hostId' query parameter if it exists.
	 *
	 * @param query
	 * A hash table of query parameters.
	 *
	 * @param hostId.
	 * If 'hostId' query parameter is found, the value is set to
	 * this variable. Otherwise, ALL_HOSTS is set.
	 */
	static void parseQueryHostId(GHashTable *query, uint64_t &hostId);
	static void parseQueryTriggerId(GHashTable *query, uint64_t &triggerId);

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
	static void handlerGetServer(RestJob *job);
	static void handlerGetHost(RestJob *job);
	static void handlerGetTrigger(RestJob *job);
	static void handlerGetEvent(RestJob *job);
	static void handlerGetItem(RestJob *job);

	static void handlerAction(RestJob *job);
	static void handlerGetAction(RestJob *job);
	static void handlerPostAction(RestJob *job);
	static void handlerDeleteAction(RestJob *job);

	static void handlerUser(RestJob *job);
	static void handlerGetUser(RestJob *job);
	static void handlerPostUser(RestJob *job);
	static void handlerDeleteUser(RestJob *job);

	void itemFetchedCallback(ClosureBase *closure);

	/**
	 * Get the SessionInfo instance.
	 * NOTE: This function doesn't take a lock in it. So you should 
	 *       take a lock if the other thread may accesses the session
	 *       information.
	 *
	 * @param sessionId A session ID string.
	 *
	 * @return
	 * A pointer to the SesionInfo instance when the session is found.
	 * Otherwise, NULL is returned.
	 */
	static const SessionInfo *getSessionInfo(const string &sessionId);

	static HatoholError parseUserParameter(UserInfo &userInfo,
	                                       GHashTable *query);

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

	static HatoholError parseSortOrderFromQuery(
	  DataQueryOption::SortOrder &sortOrder, GHashTable *query);
	static HatoholError parseEventParameter(EventQueryOption &option,
	                                        GHashTable *query);
private:
	struct PrivateContext;
	PrivateContext *m_ctx;

	// The body is defined in the FaceRest.cc. So this function can
	// be used only from the soruce file.
	template<typename T>
	static HatoholError getParam(
	  GHashTable *query, const char *paramName,
	  const char *scanFmt, T &dest);

	template<typename T>
	static bool getParamWithErrorReply(
	  RestJob *job, const char *paramName, const char *scanFmt,
	  T &dest, bool *exist);

	static const char *pathForTest;
	static const char *pathForLogin;
	static const char *pathForLogout;
	static const char *pathForGetOverview;
	static const char *pathForGetServer;
	static const char *pathForGetHost;
	static const char *pathForGetTrigger;
	static const char *pathForGetEvent;
	static const char *pathForGetItem;
	static const char *pathForAction;
	static const char *pathForUser;
};

#endif // FaceRest_h
