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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstring>
#include <Logger.h>
#include <Reaper.h>
#include <MutexLock.h>
#include <SmartTime.h>
#include <AtomicValue.h>
using namespace mlpl;

#include <errno.h>
#include <uuid/uuid.h>
#include <semaphore.h>

#include "FaceRest.h"
#include "JsonBuilderAgent.h"
#include "HatoholException.h"
#include "ConfigManager.h"
#include "UnifiedDataStore.h"
#include "DBClientUser.h"

int FaceRest::API_VERSION = 3;
const char *FaceRest::SESSION_ID_HEADER_NAME = "X-Hatohol-Session";
const int FaceRest::DEFAULT_NUM_WORKERS = 4;

typedef void (*RestHandler) (FaceRest::RestJob *job);

typedef uint64_t ServerID;
typedef uint64_t HostID;
typedef uint64_t TriggerID;
typedef map<HostID, string> HostNameMap;
typedef map<ServerID, HostNameMap> HostNameMaps;

typedef map<TriggerID, string> TriggerBriefMap;
typedef map<ServerID, TriggerBriefMap> TriggerBriefMaps;

static const guint DEFAULT_PORT = 33194;

const char *FaceRest::pathForTest        = "/test";
const char *FaceRest::pathForLogin       = "/login";
const char *FaceRest::pathForLogout      = "/logout";
const char *FaceRest::pathForGetOverview = "/overview";
const char *FaceRest::pathForGetServer   = "/server";
const char *FaceRest::pathForGetHost     = "/host";
const char *FaceRest::pathForGetTrigger  = "/trigger";
const char *FaceRest::pathForGetEvent    = "/event";
const char *FaceRest::pathForGetItem     = "/item";
const char *FaceRest::pathForAction      = "/action";
const char *FaceRest::pathForUser        = "/user";

static const char *MIME_HTML = "text/html";
static const char *MIME_JSON = "application/json";
static const char *MIME_JAVASCRIPT = "text/javascript";

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

enum FormatType {
	FORMAT_HTML,
	FORMAT_JSON,
	FORMAT_JSONP,
};

typedef map<string, FormatType> FormatTypeMap;
typedef FormatTypeMap::iterator FormatTypeMapIterator;
static FormatTypeMap g_formatTypeMap;

typedef map<FormatType, const char *> MimeTypeMap;
typedef MimeTypeMap::iterator   MimeTypeMapIterator;
static MimeTypeMap g_mimeTypeMap;

// Key: session ID, value: user ID
typedef map<string, SessionInfo *>           SessionIdMap;
typedef map<string, SessionInfo *>::iterator SessionIdMapIterator;

// constructor
SessionInfo::SessionInfo(void)
: userId(INVALID_USER_ID),
  loginTime(SmartTime::INIT_CURR_TIME),
  lastAccessTime(SmartTime::INIT_CURR_TIME)
{
}

struct FaceRest::PrivateContext {
	struct MainThreadCleaner;
	static bool         testMode;
	static MutexLock    lock;
	static SessionIdMap sessionIdMap;
	static const string pathForUserMe;
	guint               port;
	SoupServer         *soupServer;
	GMainContext       *gMainCtx;
	FaceRestParam      *param;
	AtomicValue<bool>   quitRequest;

	// for async mode
	bool                asyncMode;
	set<Worker *>       workers;
	queue<RestJob *>    restJobQueue;
	MutexLock           restJobLock;
	sem_t               waitJobSemaphore;

	PrivateContext(FaceRestParam *_param)
	: port(DEFAULT_PORT),
	  soupServer(NULL),
	  gMainCtx(NULL),
	  param(_param),
	  quitRequest(false),
	  asyncMode(true)
	{
		gMainCtx = g_main_context_new();
		sem_init(&waitJobSemaphore, 0, 0);
	}

	virtual ~PrivateContext()
	{
		g_main_context_unref(gMainCtx);
		sem_destroy(&waitJobSemaphore);
	}

	static string initPathForUserMe(void)
	{
		return string(FaceRest::pathForUser) + "/me";
	}

	static void insertSessionId(const string &sessionId, UserIdType userId)
	{
		SessionInfo *sessionInfo = new SessionInfo();
		sessionInfo->userId = userId;
		lock.lock();
		sessionIdMap[sessionId] = sessionInfo;
		lock.unlock();
	}

	static bool removeSessionId(const string &sessionId)
	{
		lock.lock();
		SessionIdMapIterator it = sessionIdMap.find(sessionId);
		bool found = it != sessionIdMap.end();
		if (found)
			sessionIdMap.erase(it);
		lock.unlock();
		if (!found) {
			MLPL_WARN("Failed to erase session ID: %s\n",
			          sessionId.c_str());
		}
		return found;
	}

	static const SessionInfo *getSessionInfo(const string &sessionId)
	{
		SessionIdMapIterator it = sessionIdMap.find(sessionId);
		if (it == sessionIdMap.end())
			return NULL;
		return it->second;
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
};

bool         FaceRest::PrivateContext::testMode = false;
MutexLock    FaceRest::PrivateContext::lock;
SessionIdMap FaceRest::PrivateContext::sessionIdMap;
const string FaceRest::PrivateContext::pathForUserMe =
  FaceRest::PrivateContext::initPathForUserMe();

struct FaceRest::RestJob
{
	// arguments of SoupServerCallback
	SoupMessage       *message;
	string             path;
	GHashTable        *query;
	SoupClientContext *client;

	FaceRest   *faceRest;
	RestHandler handler;

	// parsed data
	string      formatString;
	FormatType  formatType;
	const char *mimeType;
	string      resourceId; // we assume URL form is:
				// http://example.com/request/id
	string      jsonpCallbackName;
	string      sessionId;
	UserIdType  userId;
	bool        replyIsPrepared;

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

private:
	string getJsonpCallbackName(void);
	bool parseFormatType(void);
};

class FaceRest::Worker : public HatoholThreadBase {
public:
	Worker(FaceRest *faceRest)
	: m_faceRest(faceRest)
	{
	}
	virtual ~Worker()
	{
	}

	virtual void stop(void)
	{
		if (!isStarted())
			return;
		HatoholThreadBase::stop();
	}

protected:
	virtual gpointer mainThread(HatoholThreadArg *arg)
	{
		RestJob *job;
		MLPL_INFO("start face-rest worker\n");
		while ((job = waitNextJob())) {
			launchHandlerInTryBlock(job);
			finishRestJobIfNeeded(job);
		}
		MLPL_INFO("exited face-rest worker\n");
		return NULL;
	}

private:
	RestJob *waitNextJob(void)
	{
		while (m_faceRest->m_ctx->waitJob()) {
			RestJob *job = m_faceRest->m_ctx->popJob();
			if (job)
				return job;
		}
		return NULL;
	}

	FaceRest *m_faceRest;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void FaceRest::init(void)
{
	g_formatTypeMap["html"] = FORMAT_HTML;
	g_formatTypeMap["json"] = FORMAT_JSON;
	g_formatTypeMap["jsonp"] = FORMAT_JSONP;

	g_mimeTypeMap[FORMAT_HTML] = MIME_HTML;
	g_mimeTypeMap[FORMAT_JSON] = MIME_JSON;
	g_mimeTypeMap[FORMAT_JSONP] = MIME_JAVASCRIPT;
}

void FaceRest::reset(const CommandLineArg &arg)
{
	bool foundTestMode = false;
	for (size_t i = 0; i < arg.size(); i++) {
		if (arg[i] == "--test-mode")
			foundTestMode = true;
	}

	if (foundTestMode)
		MLPL_INFO("Run as a test mode.\n");
	PrivateContext::testMode = foundTestMode;
}

bool FaceRest::isTestMode(void)
{
	return PrivateContext::testMode;
}

FaceRest::FaceRest(CommandLineArg &cmdArg, FaceRestParam *param)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(param);

	DBClientConfig dbConfig;
	int port = dbConfig.getFaceRestPort();
	if (port != 0 && Utils::isValidPort(port))
		m_ctx->port = port;

	for (size_t i = 0; i < cmdArg.size(); i++) {
		string &cmd = cmdArg[i];
		if (cmd == "--face-rest-port")
			i = parseCmdArgPort(cmdArg, i);
	}
	MLPL_INFO("started face-rest, port: %d\n", m_ctx->port);
}

FaceRest::~FaceRest()
{
	// wait for the finish of the thread
	stop();

	MLPL_INFO("FaceRest: stop process: started.\n");
	if (m_ctx->soupServer) {
		SoupSocket *sock = soup_server_get_listener(m_ctx->soupServer);
		soup_socket_disconnect(sock);
		g_object_unref(m_ctx->soupServer);
	}
	MLPL_INFO("FaceRest: stop process: completed.\n");

	if (m_ctx)
		delete m_ctx;
}

void FaceRest::stop(void)
{
	if (isStarted()) {
		m_ctx->quitRequest.set(true);

		// To return g_main_context_iteration() in mainThread()
		struct IterAlarm {
			static gboolean task(gpointer data) {
				return G_SOURCE_REMOVE;
			}
		};
		Utils::setGLibIdleEvent(IterAlarm::task, NULL, m_ctx->gMainCtx);
	}

	HatoholThreadBase::stop();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
struct FaceRest::PrivateContext::MainThreadCleaner {
	PrivateContext *ctx;
	bool running;

	MainThreadCleaner(PrivateContext *_ctx)
	: ctx(_ctx),
	  running(false)
	{
	}

	static void callgate(MainThreadCleaner *obj)
	{
		obj->run();
	}

	void run(void)
	{
		if (ctx->soupServer && running)
			soup_server_quit(ctx->soupServer);
	}
};

typedef struct HandlerClosure
{
	FaceRest *m_faceRest;
	RestHandler m_handler;
	HandlerClosure(FaceRest *faceRest, RestHandler handler)
	: m_faceRest(faceRest), m_handler(handler)
	{
	}
} HandlerClosure;

static void deleteHandlerClosure(gpointer data)
{
	HandlerClosure *arg = static_cast<HandlerClosure *>(data);
	delete arg;
}

bool FaceRest::isAsyncMode(void)
{
	return m_ctx->asyncMode;
}

void FaceRest::startWorkers(void)
{
	for (int i = 0; i < DEFAULT_NUM_WORKERS; i++) {
		Worker *worker = new Worker(this);
		worker->start();
		m_ctx->workers.insert(worker);
	}
}

void FaceRest::stopWorkers(void)
{
	set<Worker *> &workers = m_ctx->workers;
	set<Worker *>::iterator it;
	for (it = workers.begin(); it != workers.end(); ++it) {
		// to break Worker::waitNextJob()
		sem_post(&m_ctx->waitJobSemaphore);
	}
	for (it = workers.begin(); it != workers.end(); ++it) {
		Worker *worker = *it;
		// destructor will call stop()
		delete worker;
	}
	workers.clear();
}

gpointer FaceRest::mainThread(HatoholThreadArg *arg)
{
	PrivateContext::MainThreadCleaner cleaner(m_ctx);
	Reaper<PrivateContext::MainThreadCleaner>
	   reaper(&cleaner, PrivateContext::MainThreadCleaner::callgate);

	m_ctx->soupServer = soup_server_new(SOUP_SERVER_PORT, m_ctx->port,
	                                    SOUP_SERVER_ASYNC_CONTEXT,
	                                    m_ctx->gMainCtx, NULL);
	HATOHOL_ASSERT(m_ctx->soupServer,
	               "failed: soup_server_new: %u, errno: %d\n",
	               m_ctx->port, errno);
	soup_server_add_handler(m_ctx->soupServer, NULL,
	                        handlerDefault, this, NULL);
	soup_server_add_handler(m_ctx->soupServer, "/hello.html",
	                        queueRestJob,
	                        new HandlerClosure(this, &handlerHelloPage),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, "/test",
	                        queueRestJob,
	                        new HandlerClosure(this, handlerTest),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForLogin,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerLogin),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForLogout,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerLogout),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForGetOverview,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerGetOverview),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForGetServer,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerGetServer),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForGetHost,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerGetHost),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForGetTrigger,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerGetTrigger),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForGetEvent,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerGetEvent),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForGetItem,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerGetItem),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForAction,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerAction),
				deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForUser,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerUser),
				deleteHandlerClosure);
	if (m_ctx->param)
		m_ctx->param->setupDoneNotifyFunc();
	soup_server_run_async(m_ctx->soupServer);
	cleaner.running = true;

	if (isAsyncMode())
		startWorkers();

	while (!m_ctx->quitRequest.get())
		g_main_context_iteration(m_ctx->gMainCtx, TRUE);

	if (isAsyncMode())
		stopWorkers();

	MLPL_INFO("exited face-rest\n");
	return NULL;
}

size_t FaceRest::parseCmdArgPort(CommandLineArg &cmdArg, size_t idx)
{
	if (idx == cmdArg.size() - 1) {
		MLPL_ERR("needs port number.");
		return idx;
	}

	idx++;
	string &port_str = cmdArg[idx];
	int port = atoi(port_str.c_str());
	if (!Utils::isValidPort(port, false)) {
		MLPL_ERR("invalid port: %s, %d\n", port_str.c_str(), port);
		return idx;
	}

	m_ctx->port = port;
	return idx;
}

void FaceRest::addHatoholError(JsonBuilderAgent &agent,
                               const HatoholError &err)
{
	agent.add("apiVersion", API_VERSION);
	agent.add("errorCode", err.getCode());
	if (!err.getOptionMessage().empty())
		agent.add("optionMessages", err.getOptionMessage().c_str());
}

void FaceRest::replyError(RestJob *job,
                          const HatoholError &hatoholError)
{
	replyError(job, hatoholError.getCode(),
	           hatoholError.getOptionMessage());
}

void FaceRest::replyError(RestJob *job,
                          const HatoholErrorCode &errorCode,
                          const string &optionMessage)
{
	if (optionMessage.empty()) {
		MLPL_INFO("reply error: %d\n", errorCode);
	} else {
		MLPL_INFO("reply error: %d, %s",
		          errorCode, optionMessage.c_str());
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, errorCode);
	agent.endObject();
	string response = agent.generate();
	if (!job->jsonpCallbackName.empty())
		response = wrapForJsonp(response, job->jsonpCallbackName);
	soup_message_headers_set_content_type(job->message->response_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(job->message->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(job->message, SOUP_STATUS_OK);

	job->replyIsPrepared = true;
}

string FaceRest::wrapForJsonp(const string &jsonBody,
                              const string &callbackName)
{
	string jsonp = callbackName;
	jsonp += "(";
	jsonp += jsonBody;
	jsonp += ")";
	return jsonp;
}

void FaceRest::replyJsonData(JsonBuilderAgent &agent, RestJob *job)
{
	string response = agent.generate();
	if (!job->jsonpCallbackName.empty())
		response = wrapForJsonp(response, job->jsonpCallbackName);
	soup_message_headers_set_content_type(job->message->response_headers,
	                                      job->mimeType, NULL);
	soup_message_body_append(job->message->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(job->message, SOUP_STATUS_OK);

	job->replyIsPrepared = true;
}

const SessionInfo *FaceRest::getSessionInfo(const string &sessionId)
{
	return PrivateContext::getSessionInfo(sessionId);
}

void FaceRest::parseQueryServerId(GHashTable *query, uint32_t &serverId)
{
	serverId = ALL_SERVERS;
	if (!query)
		return;
	gchar *value = (gchar *)g_hash_table_lookup(query, "serverId");
	if (!value)
		return;

	uint32_t svId;
	if (sscanf(value, "%"PRIu32, &svId) == 1)
		serverId = svId;
	else
		MLPL_INFO("Invalid requested ID: %s\n", value);
}

void FaceRest::parseQueryHostId(GHashTable *query, uint64_t &hostId)
{
	hostId = ALL_HOSTS;
	if (!query)
		return;
	gchar *value = (gchar *)g_hash_table_lookup(query, "hostId");
	if (!value)
		return;

	uint64_t id;
	if (sscanf(value, "%"PRIu64, &id) == 1)
		hostId = id;
	else
		MLPL_INFO("Invalid requested ID: %s\n", value);
}

void FaceRest::parseQueryTriggerId(GHashTable *query, uint64_t &triggerId)
{
	triggerId = ALL_TRIGGERS;
	if (!query)
		return;
	gchar *value = (gchar *)g_hash_table_lookup(query, "triggerId");
	if (!value)
		return;

	uint64_t id;
	if (sscanf(value, "%"PRIu64, &id) == 1)
		triggerId = id;
	else
		MLPL_INFO("Invalid requested ID: %s\n", value);
}

// handlers
void FaceRest::handlerDefault(SoupServer *server, SoupMessage *msg,
                              const char *path, GHashTable *query,
                              SoupClientContext *client, gpointer user_data)
{
	MLPL_DBG("Default handler: path: %s, method: %s\n",
	         path, msg->method);
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}

FaceRest::RestJob::RestJob
  (FaceRest *_faceRest, RestHandler _handler, SoupMessage *_msg,
   const char *_path, GHashTable *_query, SoupClientContext *_client)
: message(_msg), path(_path ? _path : ""), query(_query), client(_client),
  faceRest(_faceRest), handler(_handler), mimeType(NULL),
  replyIsPrepared(false)
{
	if (query)
		g_hash_table_ref(query);

	// Since life-span of other libsoup's objects should always be longer
	// than this object and shoube be managed by libsoup, we don't
	// inclement reference count of them.
}

FaceRest::RestJob::~RestJob()
{
	if (query)
		g_hash_table_unref(query);
}


string FaceRest::RestJob::getJsonpCallbackName(void)
{
	if (formatType != FORMAT_JSONP)
		return "";
	gpointer value = g_hash_table_lookup(query, "callback");
	if (!value)
		THROW_HATOHOL_EXCEPTION("Not found parameter: callback");

	const char *callbackName = (const char *)value;
	string errMsg;
	if (!Utils::validateJSMethodName(callbackName, errMsg)) {
		THROW_HATOHOL_EXCEPTION(
		  "Invalid callback name: %s", errMsg.c_str());
	}
	return callbackName;
}

bool FaceRest::RestJob::parseFormatType(void)
{
	formatString.clear();
	if (!query) {
		formatType = FORMAT_JSON;
		return true;
	}

	gchar *format = (gchar *)g_hash_table_lookup(query, "fmt");
	if (!format) {
		formatType = FORMAT_JSON; // default value
		return true;
	}
	formatString = format;

	FormatTypeMapIterator fmtIt = g_formatTypeMap.find(format);
	if (fmtIt == g_formatTypeMap.end())
		return false;
	formatType = fmtIt->second;
	return true;
}

bool FaceRest::RestJob::prepare(void)
{
	const char *_sessionId =
	   soup_message_headers_get_one(message->request_headers,
	                                SESSION_ID_HEADER_NAME);
	sessionId = _sessionId ? _sessionId : "";

	if (sessionId.empty()) {
		// We should return an error. But now, we just set
		// USER_ID_ADMIN to keep compatiblity until the user privilege
		// feature is completely implemnted.
		if (path != pathForLogin)
			userId = USER_ID_ADMIN;
		else
			userId = INVALID_USER_ID;
	} else {
		PrivateContext::lock.lock();
		const SessionInfo *sessionInfo = getSessionInfo(sessionId);
		if (!sessionInfo) {
			PrivateContext::lock.unlock();
			replyError(this, HTERR_NOT_FOUND_SESSION_ID);
			return false;
		}
		userId = sessionInfo->userId;
		PrivateContext::lock.unlock();
	}

	// We expect URIs  whose style are the following.
	//
	// Examples:
	// http://localhost:33194/action
	// http://localhost:33194/action?fmt=json
	// http://localhost:33194/action/2345?fmt=html

	// a format type
	if (!parseFormatType()) {
		REPLY_ERROR(this, HTERR_UNSUPORTED_FORMAT,
		            "%s", formatString.c_str());
		return false;
	}

	// ID
	StringVector pathElemVect;
	StringUtils::split(pathElemVect, path, '/');
	if (pathElemVect.size() >= 2)
		resourceId = pathElemVect[1];

	// MIME
	MimeTypeMapIterator mimeIt = g_mimeTypeMap.find(formatType);
	HATOHOL_ASSERT(
	  mimeIt != g_mimeTypeMap.end(),
	  "Invalid formatType: %d, %s", formatType, path.c_str());
	mimeType = mimeIt->second;

	// jsonp callback name
	jsonpCallbackName = getJsonpCallbackName();

	return true;
}

void FaceRest::RestJob::pauseResponse(void)
{
	soup_server_pause_message(server(), message);
}

struct UnpauseContext {
	SoupServer *server;
	SoupMessage *message;
};

static gboolean idleUnpause(gpointer data)
{
	UnpauseContext *ctx = static_cast<UnpauseContext *>(data);
	soup_server_unpause_message(ctx->server,
				    ctx->message);
	delete ctx;
	return FALSE;
}

void FaceRest::RestJob::unpauseResponse(void)
{
	if (g_main_context_acquire(gMainContext())) {
		// FaceRest thread
		soup_server_unpause_message(server(), message);
		g_main_context_release(gMainContext());
	} else {
		// Other threads
		UnpauseContext *unpauseContext = new UnpauseContext;
		unpauseContext->server = server();
		unpauseContext->message = message;
		soup_add_completion(gMainContext(), idleUnpause,
				    unpauseContext);
	}
}

static void copyHashTable (gpointer key, gpointer data, gpointer user_data)
{
	GHashTable *dest = static_cast<GHashTable *>(user_data);
	g_hash_table_insert(dest,
			    g_strdup(static_cast<gchar*>(key)),
			    g_strdup(static_cast<gchar*>(data)));
}

void FaceRest::queueRestJob
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *_query, SoupClientContext *client, gpointer user_data)
{
	GHashTable *query = _query;
	Reaper<GHashTable> postQueryReaper;
	if (strcasecmp(msg->method, "POST") == 0) {
		// The POST request contains query parameters in the body
		// according to application/x-www-form-urlencoded.
		query = soup_form_decode(msg->request_body->data);
	} else {
		GHashTable *dest = g_hash_table_new_full(g_str_hash,
							 g_str_equal,
							 g_free, g_free);
		if (query)
			g_hash_table_foreach(query, copyHashTable, dest);
		query = dest;
	}
	postQueryReaper.set(query, g_hash_table_unref);

	HandlerClosure *closure = static_cast<HandlerClosure *>(user_data);
	FaceRest *face = closure->m_faceRest;
	RestJob *job = new RestJob(face, closure->m_handler,
				   msg, path, query, client);
	if (!job->prepare())
		return;

	job->pauseResponse();

	if (face->isAsyncMode()) {
		face->m_ctx->pushJob(job);
	} else {
		launchHandlerInTryBlock(job);
		finishRestJobIfNeeded(job);
	}
}

void FaceRest::finishRestJobIfNeeded(RestJob *job)
{
	if (job->replyIsPrepared) {
		job->unpauseResponse();
		delete job;
	}
}

void FaceRest::launchHandlerInTryBlock(RestJob *job)
{
	try {
		(*job->handler)(job);
	} catch (const HatoholException &e) {
		REPLY_ERROR(job, HTERR_GOT_EXCEPTION,
		            "%s", e.getFancyMessage().c_str());
	}
}

void FaceRest::handlerHelloPage(RestJob *job)
{
	string response;
	const char *pageTemplate =
	  "<html>"
	  "HATOHOL Server ver. %s"
	  "</html>";
	response = StringUtils::sprintf(pageTemplate, PACKAGE_VERSION);
	soup_message_body_append(job->message->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(job->message, SOUP_STATUS_OK);
	job->replyIsPrepared = true;
}

static void addOverviewEachServer(JsonBuilderAgent &agent,
                                  MonitoringServerInfo &svInfo)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	agent.add("serverId", svInfo.id);
	agent.add("serverHostName", svInfo.hostName);
	agent.add("serverIpAddr", svInfo.ipAddress);
	agent.add("serverNickname", svInfo.nickname);

	// TODO: This implementeation is not effective.
	//       We should add a function only to get the number of list.
	HostInfoList hostInfoList;
	dataStore->getHostList(hostInfoList, svInfo.id);
	agent.add("numberOfHosts", hostInfoList.size());

	ItemInfoList itemInfoList;
	dataStore->fetchItems(svInfo.id);
	dataStore->getItemList(itemInfoList, svInfo.id);
	agent.add("numberOfItems", itemInfoList.size());

	TriggerInfoList triggerInfoList;
	dataStore->getTriggerList(triggerInfoList, svInfo.id);
	agent.add("numberOfTriggers", triggerInfoList.size());

	// TODO: These elements should be fixed
	// after the funtion concerned is added
	agent.add("numberOfUsers", 0);
	agent.add("numberOfOnlineUsers", 0);
	agent.add("numberOfMonitoredItemsPerSecond", 0);

	// HostGroups
	// TODO: We temtatively returns 'No group'. We should fix it
	//       after host group is supported in Hatohol server.
	agent.startObject("hostGroups");
	size_t numHostGroup = 1;
	uint64_t hostGroupIds[1] = {0};
	for (size_t i = 0; i < numHostGroup; i++) {
		agent.startObject(StringUtils::toString(hostGroupIds[i]));
		agent.add("name", "No group");
		agent.endObject();
	}
	agent.endObject();

	// SystemStatus
	agent.startArray("systemStatus");
	for (size_t i = 0; i < numHostGroup; i++) {
		uint64_t hostGroupId = hostGroupIds[i];
		for (int severity = 0;
		     severity < NUM_TRIGGER_SEVERITY; severity++) {
			agent.startObject();
			agent.add("hostGroupId", hostGroupId);
			agent.add("severity", severity);
			agent.add(
			  "numberOfHosts",
			  dataStore->getNumberOfTriggers
			    (svInfo.id, hostGroupId,
			     (TriggerSeverityType)severity));
			agent.endObject();
		}
	}
	agent.endArray();

	// HostStatus
	agent.startArray("hostStatus");
	for (size_t i = 0; i < numHostGroup; i++) {
		uint64_t hostGroupId = hostGroupIds[i];
		uint32_t svid = svInfo.id;
		agent.startObject();
		agent.add("hostGroupId", hostGroupId);
		agent.add("numberOfGoodHosts",
		          dataStore->getNumberOfGoodHosts(svid, hostGroupId));
		agent.add("numberOfBadHosts",
		          dataStore->getNumberOfBadHosts(svid, hostGroupId));
		agent.endObject();
	}
	agent.endArray();
}

static void addOverview(JsonBuilderAgent &agent)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	configManager->getTargetServers(monitoringServers);
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	agent.add("numberOfServers", monitoringServers.size());
	agent.startArray("serverStatus");
	for (; it != monitoringServers.end(); ++it) {
		agent.startObject();
		addOverviewEachServer(agent, *it);
		agent.endObject();
	}
	agent.endArray();

	agent.startArray("badServers");
	// TODO: implemented
	agent.endArray();
}

static void addServers(JsonBuilderAgent &agent, uint32_t targetServerId)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	configManager->getTargetServers(monitoringServers, targetServerId);

	agent.add("numberOfServers", monitoringServers.size());
	agent.startArray("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (; it != monitoringServers.end(); ++it) {
		MonitoringServerInfo &serverInfo = *it;
		agent.startObject();
		agent.add("id", serverInfo.id);
		agent.add("type", serverInfo.type);
		agent.add("hostName", serverInfo.hostName);
		agent.add("ipAddress", serverInfo.ipAddress);
		agent.add("nickname", serverInfo.nickname);
		agent.endObject();
	}
	agent.endArray();
}

static void addHosts(JsonBuilderAgent &agent,
                     uint32_t targetServerId, uint64_t targetHostId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostInfoList hostInfoList;
	dataStore->getHostList(hostInfoList, targetServerId, targetHostId);

	agent.add("numberOfHosts", hostInfoList.size());
	agent.startArray("hosts");
	HostInfoListIterator it = hostInfoList.begin();
	for (; it != hostInfoList.end(); ++it) {
		HostInfo &hostInfo = *it;
		agent.startObject();
		agent.add("id", hostInfo.id);
		agent.add("serverId", hostInfo.serverId);
		agent.add("hostName", hostInfo.hostName);
		agent.endObject();
	}
	agent.endArray();
}

static string getHostName(const ServerID serverId, const HostID hostId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	string hostName;
	HostInfoList hostInfoList;
	dataStore->getHostList(hostInfoList, serverId, hostId);
	if (hostInfoList.empty()) {
		MLPL_WARN("Failed to get HostInfo: "
		          "%"PRIu64", %"PRIu64"\n",
		          serverId, hostId);
	} else {
		HostInfo &hostInfo = *hostInfoList.begin();
		hostName = hostInfo.hostName;
	}
	return hostName;
}

static void addHostsIdNameHash(
  JsonBuilderAgent &agent, MonitoringServerInfo &serverInfo,
  HostNameMaps &hostMaps, bool lookupHostName = false)
{
	HostNameMaps::iterator server_it = hostMaps.find(serverInfo.id);
	agent.startObject("hosts");
	ServerID serverId = server_it->first;
	HostNameMap &hosts = server_it->second;
	HostNameMap::iterator it = hosts.begin();
	for (; server_it != hostMaps.end() && it != hosts.end(); it++) {
		HostID hostId = it->first;
		string &hostName = it->second;
		if (lookupHostName)
			hostName = getHostName(serverId, hostId);
		agent.startObject(StringUtils::toString(hostId));
		agent.add("name", hostName);
		agent.endObject();
	}
	agent.endObject();
}

static string getTriggerBrief(
  const ServerID serverId, const TriggerID triggerId)
{
	// TODO: use UnifiedDataStore
	DBClientHatohol dbHatohol;
	string triggerBrief;
	TriggerInfo triggerInfo;
	bool succeeded = dbHatohol.getTriggerInfo(triggerInfo,
	                                          serverId, triggerId);
	if (!succeeded) {
		MLPL_WARN("Failed to get TriggerInfo: "
		          "%"PRIu64", %"PRIu64"\n",
		          serverId, triggerId);
	} else {
		triggerBrief = triggerInfo.brief;
	}
	return triggerBrief;
}

static void addTriggersIdBriefHash(
  JsonBuilderAgent &agent, MonitoringServerInfo &serverInfo,
  TriggerBriefMaps &triggerMaps, bool lookupTriggerBrief = false)
{
	TriggerBriefMaps::iterator server_it = triggerMaps.find(serverInfo.id);
	agent.startObject("triggers");
	ServerID serverId = server_it->first;
	TriggerBriefMap &triggers = server_it->second;
	TriggerBriefMap::iterator it = triggers.begin();
	for (; server_it != triggerMaps.end() && it != triggers.end(); it++) {
		TriggerID triggerId = it->first;
		string &triggerBrief = it->second;
		if (lookupTriggerBrief)
			triggerBrief = getTriggerBrief(serverId, triggerId);
		agent.startObject(StringUtils::toString(triggerId));
		agent.add("brief", triggerBrief);
		agent.endObject();
	}
	agent.endObject();
}

static void addServersIdNameHash(
  JsonBuilderAgent &agent,
  HostNameMaps *hostMaps = NULL, bool lookupHostName = false,
  TriggerBriefMaps *triggerMaps = NULL, bool lookupTriggerBrief = false)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	configManager->getTargetServers(monitoringServers);

	agent.startObject("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (; it != monitoringServers.end(); ++it) {
		MonitoringServerInfo &serverInfo = *it;
		agent.startObject(StringUtils::toString(serverInfo.id));
		agent.add("name", serverInfo.hostName);
		if (hostMaps) {
			addHostsIdNameHash(agent, serverInfo,
			                   *hostMaps, lookupHostName);
		}
		if (triggerMaps) {
			addTriggersIdBriefHash(agent, serverInfo, *triggerMaps,
			                       lookupTriggerBrief);
		}
		agent.endObject();
	}
	agent.endObject();
}

void FaceRest::handlerTest(RestJob *job)
{
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	if (PrivateContext::testMode)
		agent.addTrue("testMode");
	else
		agent.addFalse("testMode");

	if (string(job->path) == "/test" &&
	    string(job->message->method) == "POST")
	{
		agent.startObject("queryData");
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init(&iter, job->query);
		while (g_hash_table_iter_next(&iter, &key, &value)) {
			agent.add(string((const char *)key),
			          string((const char *)value));
		}
		agent.endObject(); // queryData
		agent.endObject(); // top level
		replyJsonData(agent, job);
		return;
	} 

	if (string(job->path) == "/test/error") {
		agent.endObject(); // top level
		replyError(job, HTERR_ERROR_TEST);
		return;
	} 

	if (string(job->path) == "/test/user" &&
	    string(job->message->method) == "POST")
	{
		RETURN_IF_NOT_TEST_MODE(job);
		UserQueryOption option;
		option.setUserId(USER_ID_ADMIN);
		HatoholError err = updateOrAddUser(job->query, option);
		if (err != HTERR_OK) {
			replyError(job, err);
			return;
		}
	}

	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerLogin(RestJob *job)
{
	gchar *user = (gchar *)g_hash_table_lookup(job->query, "user");
	if (!user) {
		MLPL_INFO("Not found: user\n");
		replyError(job, HTERR_AUTH_FAILED);
		return;
	}
	gchar *password = (gchar *)g_hash_table_lookup(job->query, "password");
	if (!password) {
		MLPL_INFO("Not found: password\n");
		replyError(job, HTERR_AUTH_FAILED);
		return;
	}

	DBClientUser dbUser;
	UserIdType userId = dbUser.getUserId(user, password);
	if (userId == INVALID_USER_ID) {
		MLPL_INFO("Failed to authenticate: %s.\n", user);
		replyError(job, HTERR_AUTH_FAILED);
		return;
	}

	uuid_t sessionUuid;
	uuid_generate(sessionUuid);
	static const size_t uuidBufSize = 37;
	char uuidBuf[uuidBufSize];
	uuid_unparse(sessionUuid, uuidBuf);
	string sessionId = uuidBuf;
	PrivateContext::insertSessionId(sessionId, userId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("sessionId", sessionId);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerLogout(RestJob *job)
{
	if (!PrivateContext::removeSessionId(job->sessionId)) {
		replyError(job, HTERR_NOT_FOUND_SESSION_ID);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerGetOverview(RestJob *job)
{
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addOverview(agent);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerGetServer(RestJob *job)
{
	uint32_t targetServerId;
	parseQueryServerId(job->query, targetServerId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addServers(agent, targetServerId);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerGetHost(RestJob *job)
{
	uint32_t targetServerId;
	parseQueryServerId(job->query, targetServerId);
	uint64_t targetHostId;
	parseQueryHostId(job->query, targetHostId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addHosts(agent, targetServerId, targetHostId);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerGetTrigger(RestJob *job)
{

	uint32_t serverId;
	parseQueryServerId(job->query, serverId);
	uint64_t hostId;
	parseQueryHostId(job->query, hostId);
	uint64_t triggerId;
	parseQueryTriggerId(job->query, triggerId);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList triggerList;
	dataStore->getTriggerList(triggerList, serverId, hostId, triggerId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfTriggers", triggerList.size());
	agent.startArray("triggers");
	TriggerInfoListIterator it = triggerList.begin();
	HostNameMaps hostMaps;
	for (; it != triggerList.end(); ++it) {
		TriggerInfo &triggerInfo = *it;
		agent.startObject();
		agent.add("id",       triggerInfo.id);
		agent.add("status",   triggerInfo.status);
		agent.add("severity", triggerInfo.severity);
		agent.add("lastChangeTime", triggerInfo.lastChangeTime.tv_sec);
		agent.add("serverId", triggerInfo.serverId);
		agent.add("hostId",   triggerInfo.hostId);
		agent.add("brief",    triggerInfo.brief);
		agent.endObject();

		hostMaps[triggerInfo.serverId][triggerInfo.hostId]
		  = triggerInfo.hostName;
	}
	agent.endArray();
	addServersIdNameHash(agent, &hostMaps);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerGetEvent(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	EventInfoList eventList;
	EventQueryOption option;
	option.setUserId(job->userId);
	HatoholError err = parseEventParameter(option, job->query);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}
	err = dataStore->getEventList(eventList, option);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfEvents", eventList.size());
	agent.startArray("events");
	EventInfoListIterator it = eventList.begin();
	HostNameMaps hostMaps;
	for (; it != eventList.end(); ++it) {
		EventInfo &eventInfo = *it;
		agent.startObject();
		agent.add("unifiedId", eventInfo.unifiedId);
		agent.add("serverId",  eventInfo.serverId);
		agent.add("time",      eventInfo.time.tv_sec);
		agent.add("type",      eventInfo.type);
		agent.add("triggerId", eventInfo.triggerId);
		agent.add("status",    eventInfo.status);
		agent.add("severity",  eventInfo.severity);
		agent.add("hostId",    eventInfo.hostId);
		agent.add("brief",     eventInfo.brief);
		agent.endObject();

		hostMaps[eventInfo.serverId][eventInfo.hostId]
		  = eventInfo.hostName;
	}
	agent.endArray();
	addServersIdNameHash(agent, &hostMaps);
	agent.endObject();

	replyJsonData(agent, job);
}

struct GetItemClosure : Closure<FaceRest>
{
	struct FaceRest::RestJob *m_restJob;
	GetItemClosure(FaceRest *receiver,
		       callback func,
		       struct FaceRest::RestJob *restJob)
	: Closure(receiver, func), m_restJob(restJob)
	{
	}

	virtual ~GetItemClosure()
	{
		delete m_restJob;
	}
};

void FaceRest::replyGetItem(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	ItemInfoList itemList;
	dataStore->getItemList(itemList);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfItems", itemList.size());
	agent.startArray("items");
	ItemInfoListIterator it = itemList.begin();
	for (; it != itemList.end(); ++it) {
		ItemInfo &itemInfo = *it;
		agent.startObject();
		agent.add("serverId",  itemInfo.serverId);
		agent.add("hostId",    itemInfo.hostId);
		agent.add("brief",     itemInfo.brief.c_str());
		agent.add("lastValueTime", itemInfo.lastValueTime.tv_sec);
		agent.add("lastValue", itemInfo.lastValue);
		agent.add("prevValue", itemInfo.prevValue);
		agent.add("itemGroupName", itemInfo.itemGroupName);
		agent.endObject();
	}
	agent.endArray();
	addServersIdNameHash(agent);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::itemFetchedCallback(ClosureBase *closure)
{
	GetItemClosure *data = dynamic_cast<GetItemClosure*>(closure);
	RestJob *job = data->m_restJob;
	replyGetItem(job);
	job->unpauseResponse();
}

void FaceRest::handlerGetItem(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	FaceRest *face = job->faceRest;

	GetItemClosure *closure =
	  new GetItemClosure(face, &FaceRest::itemFetchedCallback, job);

	bool handled = dataStore->getItemListAsync(closure);
	if (!handled) {
		face->replyGetItem(job);
		// avoid freeing m_restJob because m_restJob will be freed at
		// queueRestJob()
		closure->m_restJob = NULL;
		delete closure;
	}
}

template <typename T>
static void setActionCondition(
  JsonBuilderAgent &agent, const ActionCondition &cond,
  const string &member, ActionConditionEnableFlag bit,
  T value)
{
		if (cond.isEnable(bit))
			agent.add(member, value);
		else
			agent.addNull(member);
}

void FaceRest::handlerAction(RestJob *job)
{
	if (strcasecmp(job->message->method, "GET") == 0) {
		handlerGetAction(job);
	} else if (strcasecmp(job->message->method, "POST") == 0) {
		handlerPostAction(job);
	} else if (strcasecmp(job->message->method, "DELETE") == 0) {
		handlerDeleteAction(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
					SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

void FaceRest::handlerGetAction(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	ActionDefList actionList;
	dataStore->getActionList(actionList);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfActions", actionList.size());
	agent.startArray("actions");
	HostNameMaps hostMaps;
	TriggerBriefMaps triggerMaps;
	ActionDefListIterator it = actionList.begin();
	for (; it != actionList.end(); ++it) {
		const ActionDef &actionDef = *it;
		const ActionCondition &cond = actionDef.condition;
		agent.startObject();
		agent.add("actionId",  actionDef.id);
		agent.add("enableBits", cond.enableBits);
		setActionCondition<uint32_t>(
		  agent, cond, "serverId", ACTCOND_SERVER_ID, cond.serverId);
		setActionCondition<uint64_t>(
		  agent, cond, "hostId", ACTCOND_HOST_ID, cond.hostId);
		setActionCondition<uint64_t>(
		  agent, cond, "hostGroupId", ACTCOND_HOST_GROUP_ID,
		   cond.hostGroupId);
		setActionCondition<uint64_t>(
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
		agent.endObject();
		// We don't know the host name at this point.
		// We'll get it later.
		if (cond.isEnable(ACTCOND_HOST_ID))
			hostMaps[cond.serverId][cond.hostId] = "";
		if (cond.isEnable(ACTCOND_TRIGGER_ID))
			triggerMaps[cond.serverId][cond.triggerId] = "";
	}
	agent.endArray();
	const bool lookupHostName = true;
	const bool lookupTriggerBrief = true;
	addServersIdNameHash(agent, &hostMaps, lookupHostName,
	                     &triggerMaps, lookupTriggerBrief);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerPostAction(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	//
	// mandatory parameters
	//
	char *value;
	bool exist;
	bool succeeded;
	ActionDef actionDef;

	// action type
	succeeded = getParamWithErrorReply<int>(
	              job, "type", "%d", (int &)actionDef.type, &exist);
	if (!succeeded)
		return;
	if (!exist) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "type");
		return;
	}
	if (!(actionDef.type == ACTION_COMMAND ||
	      actionDef.type == ACTION_RESIDENT)) {
		REPLY_ERROR(job, HTERR_INVALID_PARAMETER,
		            "type: %d\n", actionDef.type);
		return;
	}

	// command
	value = (char *)g_hash_table_lookup(job->query, "command");
	if (!value) {
		replyError(job, HTERR_NOT_FOUND_PARAMETER, "command");
		return;
	}
	actionDef.command = value;

	//
	// optional parameters
	//
	ActionCondition &cond = actionDef.condition;

	// workingDirectory
	value = (char *)g_hash_table_lookup(job->query, "workingDirectory");
	if (value) {
		actionDef.workingDir = value;
	}

	// timeout
	succeeded = getParamWithErrorReply<int>(
	              job, "timeout", "%d", actionDef.timeout, &exist);
	if (!succeeded)
		return;
	if (!exist)
		actionDef.timeout = 0;

	// serverId
	succeeded = getParamWithErrorReply<int>(
	              job, "serverId", "%d", cond.serverId, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_SERVER_ID);

	// hostId
	succeeded = getParamWithErrorReply<uint64_t>(
	              job, "hostId", "%"PRIu64, cond.hostId, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_HOST_ID);

	// hostGroupId
	succeeded = getParamWithErrorReply<uint64_t>(
	              job, "hostGroupId", "%"PRIu64, cond.hostGroupId, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_HOST_GROUP_ID);

	// triggerId
	succeeded = getParamWithErrorReply<uint64_t>(
	              job, "triggerId", "%"PRIu64, cond.triggerId, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_TRIGGER_ID);

	// triggerStatus
	succeeded = getParamWithErrorReply<int>(
	              job, "triggerStatus", "%d", cond.triggerStatus, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_TRIGGER_STATUS);

	// triggerSeverity
	succeeded = getParamWithErrorReply<int>(
	              job, "triggerSeverity", "%d", cond.triggerSeverity, &exist);
	if (!succeeded)
		return;
	if (exist) {
		cond.enable(ACTCOND_TRIGGER_SEVERITY);

		// triggerSeverityComparatorType
		succeeded = getParamWithErrorReply<int>(
		              job, "triggerSeverityCompType", "%d",
		              (int &)cond.triggerSeverityCompType, &exist);
		if (!succeeded)
			return;
		if (!exist) {
			replyError(job, HTERR_NOT_FOUND_PARAMETER, 
			           "triggerSeverityCompType");
			return;
		}
		if (!(cond.triggerSeverityCompType == CMP_EQ ||
		      cond.triggerSeverityCompType == CMP_EQ_GT)) {
			REPLY_ERROR(job, HTERR_INVALID_PARAMETER,
			            "type: %d", cond.triggerSeverityCompType);
			return;
		}
	}

	// save the obtained action
	dataStore->addAction(actionDef);

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("id", actionDef.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerDeleteAction(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	if (job->resourceId.empty()) {
		replyError(job, HTERR_NOT_FOUND_ID_IN_URL);
		return;
	}
	int actionId;
	if (sscanf(job->resourceId.c_str(), "%d", &actionId) != 1) {
		REPLY_ERROR(job, HTERR_INVALID_PARAMETER,
		            "id: %s", job->resourceId.c_str());
		return;
	}
	ActionIdList actionIdList;
	actionIdList.push_back(actionId);
	dataStore->deleteActionList(actionIdList);

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("id", job->resourceId);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerUser(RestJob *job)
{
	if (strcasecmp(job->message->method, "GET") == 0) {
		handlerGetUser(job);
	} else if (strcasecmp(job->message->method, "POST") == 0) {
		handlerPostUser(job);
	} else if (strcasecmp(job->message->method, "DELETE") == 0) {
		handlerDeleteUser(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
					SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

void FaceRest::handlerGetUser(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	UserInfoList userList;
	UserQueryOption option;
	option.setUserId(job->userId);
	if (job->path == PrivateContext::pathForUserMe)
		option.queryOnlyMyself();
	dataStore->getUserList(userList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfUsers", userList.size());
	agent.startArray("users");
	UserInfoListIterator it = userList.begin();
	for (; it != userList.end(); ++it) {
		const UserInfo &userInfo = *it;
		agent.startObject();
		agent.add("userId",  userInfo.id);
		agent.add("name", userInfo.name);
		agent.add("flags", userInfo.flags);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerPostUser(RestJob *job)
{
	// Get query parameters
	char *value;
	bool exist;
	bool succeeded;
	UserInfo userInfo;

	// name
	value = (char *)g_hash_table_lookup(job->query, "user");
	if (!value) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "user");
		return;
	}
	userInfo.name = value;

	// password
	value = (char *)g_hash_table_lookup(job->query, "password");
	if (!value) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "password");
		return;
	}
	userInfo.password = value;

	// flags
	succeeded = getParamWithErrorReply<OperationPrivilegeFlag>(
	              job, "flags", "%"FMT_OPPRVLG, userInfo.flags, &exist);
	if (!succeeded)
		return;
	if (!exist) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "flags");
		return;
	}

	// try to add
	DataQueryOption option;
	option.setUserId(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->addUser(userInfo, option);

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", userInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerDeleteUser(RestJob *job)
{
	if (job->resourceId.empty()) {
		replyError(job, HTERR_NOT_FOUND_ID_IN_URL);
		return;
	}
	int userId;
	if (sscanf(job->resourceId.c_str(), "%"FMT_USER_ID, &userId) != 1) {
		REPLY_ERROR(job, HTERR_INVALID_PARAMETER,
		            "id: %s", job->resourceId.c_str());
		return;
	}

	DataQueryOption option;
	option.setUserId(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->deleteUser(userId, option);

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", userId);
	agent.endObject();
	replyJsonData(agent, job);
}

HatoholError FaceRest::parseUserParameter(UserInfo &userInfo, GHashTable *query)
{
	char *value;

	// name
	value = (char *)g_hash_table_lookup(query, "user");
	if (!value)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "user");
	userInfo.name = value;

	// password
	value = (char *)g_hash_table_lookup(query, "password");
	if (!value) {
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "password");
	}
	userInfo.password = value;

	// flags
	HatoholError err = getParam<OperationPrivilegeFlag>(
	                     query, "flags", "%"FMT_OPPRVLG, userInfo.flags);
	if (err != HTERR_OK)
		return err;
	return HatoholError(HTERR_OK);
}

HatoholError FaceRest::updateOrAddUser(GHashTable *query,
                                       UserQueryOption &option)
{
	UserInfo userInfo;
	HatoholError err = parseUserParameter(userInfo, query);
	if (err != HTERR_OK)
		return err;

	err = option.setTargetName(userInfo.name);
	if (err != HTERR_OK)
		return err;
	UserInfoList userList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getUserList(userList, option);

	if (userList.empty()) {
		err = dataStore->addUser(userInfo, option);
	} else {
		userInfo.id = userList.begin()->id;
		err = dataStore->updateUser(userInfo, option);
	}
	if (err != HTERR_OK)
		return err;
	return HatoholError(HTERR_OK);
}

HatoholError FaceRest::parseSortOrderFromQuery(
  DataQueryOption::SortOrder &sortOrder, GHashTable *query)
{
	HatoholError err =
	   getParam<DataQueryOption::SortOrder>(query, "sortOrder", "%d",
	                                        sortOrder);
	if (err != HTERR_OK)
		return err;
	if (sortOrder != DataQueryOption::SORT_DONT_CARE &&
	    sortOrder != DataQueryOption::SORT_ASCENDING &&
	    sortOrder != DataQueryOption::SORT_DESCENDING) {
		return HatoholError(HTERR_INVALID_PARAMETER,
		                    StringUtils::sprintf("%d", sortOrder));
	}
	return HatoholError(HTERR_OK);
}

HatoholError FaceRest::parseEventParameter(EventQueryOption &option,
                                           GHashTable *query)
{
	if (!query)
		return HatoholError(HTERR_OK);

	HatoholError err;

	// sort order
	DataQueryOption::SortOrder sortOrder;
	err = parseSortOrderFromQuery(sortOrder, query);
	if (err == HTERR_OK)
		option.setSortOrder(sortOrder);
	else if (err != HTERR_NOT_FOUND_PARAMETER)
		return err;

	// maximum number
	size_t maximumNumber = 0;
	err = getParam<size_t>(query, "maximumNumber", "%zd", maximumNumber);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setMaximumNumber(maximumNumber);

	// start ID
	uint64_t startId = 0;
	err = getParam<uint64_t>(query, "startId", "%"PRIu64, startId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setStartId(startId);

	return HatoholError(HTERR_OK);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
template<typename T>
HatoholError FaceRest::getParam(
  GHashTable *query, const char *paramName, const char *scanFmt, T &dest)
{
	char *value = (char *)g_hash_table_lookup(query, paramName);
	if (!value)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, paramName);

	if (sscanf(value, scanFmt, &dest) != 1) {
		string optMsg = StringUtils::sprintf("%s: %s\n",
		                                     paramName, value);
		return HatoholError(HTERR_INVALID_PARAMETER, optMsg);
	}
	return HatoholError(HTERR_OK);
}

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
		            "%s: %s\n", paramName, value);
		return false;
	}
	return true;
}

