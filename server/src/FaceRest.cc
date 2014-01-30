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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstring>
#include <Logger.h>
#include <Reaper.h>
#include <MutexLock.h>
#include <SmartTime.h>
#include <AtomicValue.h>
#include <errno.h>
#include <uuid/uuid.h>
#include <semaphore.h>
#include "FaceRest.h"
#include "JsonBuilderAgent.h"
#include "HatoholException.h"
#include "ConfigManager.h"
#include "UnifiedDataStore.h"
#include "DBClientUser.h"
#include "DBClientConfig.h"
#include "SessionManager.h"
using namespace std;
using namespace mlpl;

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
const char *FaceRest::pathForServer      = "/server";
const char *FaceRest::pathForGetHost     = "/host";
const char *FaceRest::pathForGetTrigger  = "/trigger";
const char *FaceRest::pathForGetEvent    = "/event";
const char *FaceRest::pathForGetItem     = "/item";
const char *FaceRest::pathForAction      = "/action";
const char *FaceRest::pathForUser        = "/user";
const char *FaceRest::pathForHostgroup   = "/hostgroup";
const char *FaceRest::pathForUserRole    = "/user-role";

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

struct FaceRest::PrivateContext {
	struct MainThreadCleaner;
	static bool         testMode;
	static MutexLock    lock;
	static const string pathForUserMe;
	guint               port;
	SoupServer         *soupServer;
	GMainContext       *gMainCtx;
	FaceRestParam      *param;
	AtomicValue<bool>   quitRequest;

	// for async mode
	bool                asyncMode;
	size_t              numPreLoadWorkers;
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
	  asyncMode(true),
	  numPreLoadWorkers(DEFAULT_NUM_WORKERS)
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

	static bool isTestPath(const string &path)
	{
		size_t len = strlen(pathForTest);
		return (strncmp(path.c_str(), pathForTest, len) == 0);
	}
};

bool         FaceRest::PrivateContext::testMode = false;
MutexLock    FaceRest::PrivateContext::lock;
const string FaceRest::PrivateContext::pathForUserMe =
  FaceRest::PrivateContext::initPathForUserMe();

static const uint64_t INVALID_ID = -1;

struct FaceRest::RestJob
{
	// arguments of SoupServerCallback
	SoupMessage       *message;
	string             path;
	StringVector       pathElements;
	GHashTable        *query;
	SoupClientContext *client;

	FaceRest   *faceRest;
	RestHandler handler;

	// parsed data
	string      formatString;
	FormatType  formatType;
	const char *mimeType;
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

	string   getResourceName(int nest = 0);
	string   getResourceIdString(int nest = 0);
	uint64_t getResourceId(int nest = 0);

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

void FaceRest::setNumberOfPreLoadWorkers(size_t num)
{
	m_ctx->numPreLoadWorkers = num;
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
	for (size_t i = 0; i < m_ctx->numPreLoadWorkers; i++) {
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
	if (errno == EADDRINUSE)
		MLPL_ERR("%s", Utils::getUsingPortInfo(m_ctx->port).c_str());
	HATOHOL_ASSERT(m_ctx->soupServer,
	               "failed: soup_server_new: %u, errno: %d\n",
	               m_ctx->port, errno);
	soup_server_add_handler(m_ctx->soupServer, NULL,
	                        handlerDefault, this, NULL);
	soup_server_add_handler(m_ctx->soupServer, "/hello.html",
	                        queueRestJob,
	                        new HandlerClosure(this, &handlerHelloPage),
	                        deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForTest,
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
	soup_server_add_handler(m_ctx->soupServer, pathForServer,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerServer),
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
	soup_server_add_handler(m_ctx->soupServer, pathForHostgroup,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerGetHostgroup),
	                        deleteHandlerClosure);
	soup_server_add_handler(m_ctx->soupServer, pathForUserRole,
	                        queueRestJob,
	                        new HandlerClosure(this, handlerUserRole),
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

void FaceRest::parseQueryHostgroupId(GHashTable *query, uint64_t &hostgroupId)
{
	hostgroupId = ALL_HOST_GROUPS;
	if (!query)
		return;
	gchar *value = (gchar *)g_hash_table_lookup(query, "hostgroupId");
	if (!value)
		return;

	uint64_t id;
	if (sscanf(value, "%"PRIu64, &id) == 1)
		hostgroupId = id;
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

	bool notFoundSessionId = true;
	if (sessionId.empty()) {
		if (path == pathForLogin || PrivateContext::isTestPath(path)) {
			userId = INVALID_USER_ID;
			notFoundSessionId = false;
		}
	} else {
		SessionManager *sessionMgr = SessionManager::getInstance();
		SessionPtr session = sessionMgr->getSession(sessionId);
		if (session.hasData()) {
			notFoundSessionId = false;
			userId = session->userId;
		}
	}
	if (notFoundSessionId) {
		replyError(this, HTERR_NOT_FOUND_SESSION_ID);
		return false;
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

	// path elements
	StringUtils::split(pathElements, path, '/');

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

string FaceRest::RestJob::getResourceName(int nest)
{
	size_t idx = nest * 2;
	if (pathElements.size() > idx)
		return pathElements[idx];
	return string();
}

string FaceRest::RestJob::getResourceIdString(int nest)
{
	size_t idx = nest * 2 + 1;
	if (pathElements.size() > idx)
		return pathElements[idx];
	return string();
}

uint64_t FaceRest::RestJob::getResourceId(int nest)
{
	size_t idx = nest * 2 + 1;
	if (pathElements.size() <= idx)
		return INVALID_ID;
	uint64_t id = INVALID_ID;
	if (sscanf(pathElements[idx].c_str(), "%"PRIu64"", &id) != 1)
		return INVALID_ID;
	return id;
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
	if (StringUtils::casecmp(msg->method, "POST") ||
	    StringUtils::casecmp(msg->method, "PUT")) {
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

static void addOverviewEachServer(FaceRest::RestJob *job,
				  JsonBuilderAgent &agent,
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
	HostsQueryOption option(job->userId);
	option.setTargetServerId(svInfo.id);
	dataStore->getHostList(hostInfoList, option);
	agent.add("numberOfHosts", hostInfoList.size());

	ItemInfoList itemInfoList;
	ItemsQueryOption itemsQueryOption(job->userId);
	bool fetchItemsSynchronously = true;
	itemsQueryOption.setTargetServerId(svInfo.id);
	dataStore->getItemList(itemInfoList, itemsQueryOption,
			       ALL_ITEMS, fetchItemsSynchronously);
	agent.add("numberOfItems", itemInfoList.size());

	TriggerInfoList triggerInfoList;
	TriggersQueryOption triggersQueryOption(job->userId);
	triggersQueryOption.setTargetServerId(svInfo.id);
	dataStore->getTriggerList(triggerInfoList, triggersQueryOption);
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
			TriggersQueryOption option(job->userId);
			option.setTargetServerId(svInfo.id);
			//TODO: Currently host group isn't supported yet
			//option.setTargetHostGroupId(hostGroupId);
			agent.startObject();
			agent.add("hostGroupId", hostGroupId);
			agent.add("severity", severity);
			agent.add(
			  "numberOfTriggers",
			  dataStore->getNumberOfTriggers
			    (option, (TriggerSeverityType)severity));
			agent.endObject();
		}
	}
	agent.endArray();

	// HostStatus
	agent.startArray("hostStatus");
	for (size_t i = 0; i < numHostGroup; i++) {
		uint64_t hostGroupId = hostGroupIds[i];
		HostsQueryOption option(job->userId);
		option.setTargetServerId(svInfo.id);
		//TODO: Host group isn't supported yet
		//option.setTargetHostGroupId(hostGroupId);
		agent.startObject();
		agent.add("hostGroupId", hostGroupId);
		agent.add("numberOfGoodHosts",
		          dataStore->getNumberOfGoodHosts(option));
		agent.add("numberOfBadHosts",
		          dataStore->getNumberOfBadHosts(option));
		agent.endObject();
	}
	agent.endArray();
}

static void addOverview(FaceRest::RestJob *job, JsonBuilderAgent &agent)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(job->userId);
	configManager->getTargetServers(monitoringServers, option);
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	agent.add("numberOfServers", monitoringServers.size());
	agent.startArray("serverStatus");
	for (; it != monitoringServers.end(); ++it) {
		agent.startObject();
		addOverviewEachServer(job, agent, *it);
		agent.endObject();
	}
	agent.endArray();

	agent.startArray("badServers");
	// TODO: implemented
	agent.endArray();
}

static void addServers(FaceRest::RestJob *job, JsonBuilderAgent &agent,
		       uint32_t targetServerId)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(job->userId);
	option.setTargetServerId(targetServerId);
	configManager->getTargetServers(monitoringServers, option);

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
		agent.add("port", serverInfo.port);
		agent.endObject();
	}
	agent.endArray();
}

static void addHosts(FaceRest::RestJob *job, JsonBuilderAgent &agent,
                     uint32_t targetServerId, uint64_t targetHostId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostInfoList hostInfoList;
	HostsQueryOption option(job->userId);
	option.setTargetServerId(targetServerId);
	option.setTargetHostId(targetHostId);
	dataStore->getHostList(hostInfoList, option);

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

static string getHostName(const UserIdType userId,
			  const ServerID serverId, const HostID hostId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	string hostName;
	HostInfoList hostInfoList;
	HostsQueryOption option(userId);
	option.setTargetServerId(serverId);
	option.setTargetHostId(hostId);
	dataStore->getHostList(hostInfoList, option);
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

static void addHostsMap(
  FaceRest::RestJob *job,
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
			hostName = getHostName(job->userId, serverId, hostId);
		agent.startObject(StringUtils::toString(hostId));
		agent.add("name", hostName);
		agent.endObject();
	}
	agent.endObject();
}

static string getTriggerBrief(
  FaceRest::RestJob *job, const ServerID serverId, const TriggerID triggerId)
{
	string triggerBrief;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList triggerInfoList;
	TriggersQueryOption triggersQueryOption(job->userId);
	triggersQueryOption.setTargetServerId(serverId);
	dataStore->getTriggerList(triggerInfoList, triggersQueryOption,
				  triggerId);

	if (triggerInfoList.size() != 1) {
		MLPL_WARN("Failed to get TriggerInfo: "
		          "%"PRIu64", %"PRIu64"\n",
		          serverId, triggerId);
	} else {
		TriggerInfoListIterator it = triggerInfoList.begin();
		TriggerInfo &triggerInfo = *it;
		triggerBrief = triggerInfo.brief;
	}
	return triggerBrief;
}

static void addTriggersIdBriefHash(
  FaceRest::RestJob *job,
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
			triggerBrief = getTriggerBrief(job,
						       serverId,
						       triggerId);
		agent.startObject(StringUtils::toString(triggerId));
		agent.add("brief", triggerBrief);
		agent.endObject();
	}
	agent.endObject();
}

static void addServersMap(
  FaceRest::RestJob *job,
  JsonBuilderAgent &agent,
  HostNameMaps *hostMaps = NULL, bool lookupHostName = false,
  TriggerBriefMaps *triggerMaps = NULL, bool lookupTriggerBrief = false)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(job->userId);
	configManager->getTargetServers(monitoringServers, option);

	agent.startObject("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (; it != monitoringServers.end(); ++it) {
		MonitoringServerInfo &serverInfo = *it;
		agent.startObject(StringUtils::toString(serverInfo.id));
		agent.add("name", serverInfo.hostName);
		agent.add("type", serverInfo.type);
		agent.add("ipAddress", serverInfo.ipAddress);
		if (hostMaps) {
			addHostsMap(job, agent, serverInfo,
				    *hostMaps, lookupHostName);
		}
		if (triggerMaps) {
			addTriggersIdBriefHash(job, agent, serverInfo,
					       *triggerMaps,
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
		UserQueryOption option(USER_ID_SYSTEM);
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

	SessionManager *sessionMgr = SessionManager::getInstance();
	string sessionId = sessionMgr->create(userId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("sessionId", sessionId);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerLogout(RestJob *job)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	if (!sessionMgr->remove(job->sessionId)) {
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
	addOverview(job, agent);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerServer(RestJob *job)
{
	if (StringUtils::casecmp(job->message->method, "GET")) {
		handlerGetServer(job);
	} else if (StringUtils::casecmp(job->message->method, "POST")) {
		handlerPostServer(job);
	} else if (StringUtils::casecmp(job->message->method, "DELETE")) {
		handlerDeleteServer(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
					SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->replyIsPrepared = true;
	}
}

void FaceRest::handlerGetServer(RestJob *job)
{
	uint32_t targetServerId;
	parseQueryServerId(job->query, targetServerId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addServers(job, agent, targetServerId);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerPostServer(RestJob *job)
{
	char *charValue;
	int intValue;
	bool succeeded;
	bool exist;
	MonitoringServerInfo svInfo;

	// type
	succeeded = getParamWithErrorReply<MonitoringSystemType>(
			job, "type", "%d", svInfo.type, &exist);
	if (!succeeded)
		return;
	if (!exist) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "type");
		return;
	}

	// hostname
	charValue = (char *)g_hash_table_lookup(job->query, "hostName");
	if (!charValue) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "hostName");
		return;
	}
	svInfo.hostName = charValue;

	// ipAddress
	charValue = (char *)g_hash_table_lookup(job->query, "ipAddress");
	if (!charValue) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "ipAddress");
		return;
	}
	svInfo.ipAddress = charValue;

	// nickname
	charValue = (char *)g_hash_table_lookup(job->query, "nickname");
	if (!charValue) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "nickname");
		return;
	}
	svInfo.nickname = charValue;

	// port
	charValue = (char *)g_hash_table_lookup(job->query, "port");
	if (!charValue) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "port");
		return;
	}
	sscanf(charValue, "%d", &intValue);
	svInfo.port = intValue;

	// polling
	charValue = (char *)g_hash_table_lookup(job->query, "polling");
	if (!charValue) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "polling");
		return;
	}
	sscanf(charValue, "%d", &intValue);
	svInfo.pollingIntervalSec = intValue;

	// retry
	charValue = (char *)g_hash_table_lookup(job->query, "retry");
	if (!charValue) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "retry");
		return;
	}
	sscanf(charValue, "%d", &intValue);
	svInfo.retryIntervalSec = intValue;

	// username
	charValue = (char *)g_hash_table_lookup(job->query, "user");
	if (!charValue) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "user");
		return;
	}
	svInfo.userName = charValue;

	// password
	charValue = (char *)g_hash_table_lookup(job->query, "password");
	if (!charValue) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "password");
		return;
	}
	svInfo.password = charValue;

	// dbname
	if (svInfo.type == MONITORING_SYSTEM_NAGIOS) {
		charValue = (char *)g_hash_table_lookup(job->query, "dbName");
		if (!charValue) {
			REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "dbName");
			return;
		}
		svInfo.dbName = charValue;
	}

	OperationPrivilege privilege(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->addTargetServer(svInfo, privilege);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", svInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerDeleteServer(RestJob *job)
{
	uint64_t serverId = job->getResourceId();
	if (serverId == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
			    "id: %s", job->getResourceIdString().c_str());
		return;
	}

	OperationPrivilege privilege(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->deleteTargetServer(serverId, privilege);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", serverId);
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
	addHosts(job, agent, targetServerId, targetHostId);
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
	TriggersQueryOption option(job->userId);
	option.setTargetServerId(serverId);
	option.setTargetHostId(hostId);
	dataStore->getTriggerList(triggerList, option, triggerId);

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
	addServersMap(job, agent, &hostMaps);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerGetEvent(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	EventInfoList eventList;
	EventsQueryOption option(job->userId);
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
	addServersMap(job, agent, &hostMaps);
	agent.endObject();

	replyJsonData(agent, job);
}

struct GetItemClosure : Closure<FaceRest>
{
	struct FaceRest::RestJob *m_restJob;
	GetItemClosure(FaceRest *receiver,
		       callback func,
		       struct FaceRest::RestJob *restJob)
	: Closure<FaceRest>::Closure(receiver, func), m_restJob(restJob)
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
	ItemsQueryOption option(job->userId);
	dataStore->getItemList(itemList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfItems", itemList.size());
	agent.startArray("items");
	ItemInfoListIterator it = itemList.begin();
	HostNameMaps hostMaps;
	for (; it != itemList.end(); ++it) {
		ItemInfo &itemInfo = *it;
		agent.startObject();
		agent.add("id",        itemInfo.id);
		agent.add("serverId",  itemInfo.serverId);
		agent.add("hostId",    itemInfo.hostId);
		agent.add("brief",     itemInfo.brief.c_str());
		agent.add("lastValueTime", itemInfo.lastValueTime.tv_sec);
		agent.add("lastValue", itemInfo.lastValue);
		agent.add("prevValue", itemInfo.prevValue);
		agent.add("itemGroupName", itemInfo.itemGroupName);
		agent.endObject();

		// We don't know the host name at this point.
		// We'll get it later.
		hostMaps[itemInfo.serverId][itemInfo.hostId] = "";
	}
	agent.endArray();
	const bool lookupHostName = true;
	addServersMap(job, agent, &hostMaps, lookupHostName);
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

	bool handled = dataStore->fetchItemsAsync(closure);
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
	if (StringUtils::casecmp(job->message->method, "GET")) {
		handlerGetAction(job);
	} else if (StringUtils::casecmp(job->message->method, "POST")) {
		handlerPostAction(job);
	} else if (StringUtils::casecmp(job->message->method, "DELETE")) {
		handlerDeleteAction(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
					SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->replyIsPrepared = true;
	}
}

void FaceRest::handlerGetAction(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	ActionDefList actionList;
	OperationPrivilege privilege(job->userId);
	HatoholError err = dataStore->getActionList(actionList, privilege);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err != HTERR_OK) {
		agent.endObject();
		replyJsonData(agent, job);
		return;
	}
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
		agent.add("ownerUserId", actionDef.ownerUserId);
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
	addServersMap(job, agent, &hostMaps, lookupHostName,
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

	// ownerUserId
	succeeded = getParamWithErrorReply<int>(
	              job, "ownerUserId", "%d", actionDef.ownerUserId, &exist);
	if (!succeeded)
		return;
	if (!exist)
		actionDef.ownerUserId = job->userId;

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
	OperationPrivilege privilege(job->userId);
	HatoholError err = dataStore->addAction(actionDef, privilege);

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", actionDef.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerDeleteAction(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	uint64_t actionId = job->getResourceId();
	if (actionId == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
			    "id: %s", job->getResourceIdString().c_str());
		return;
	}
	ActionIdList actionIdList;
	actionIdList.push_back(actionId);
	OperationPrivilege privilege(job->userId);
	HatoholError err = dataStore->deleteActionList(actionIdList, privilege);

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	if (err == HTERR_OK)
		agent.add("id", actionId);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerUser(RestJob *job)
{
	// handle sub-resources
	string resourceName = job->getResourceName(1);
	if (StringUtils::casecmp(resourceName, "access-info")) {
		handlerAccessInfo(job);
		return;
	}

	// handle "user" resource itself
	if (StringUtils::casecmp(job->message->method, "GET")) {
		handlerGetUser(job);
	} else if (StringUtils::casecmp(job->message->method, "POST")) {
		handlerPostUser(job);
	} else if (StringUtils::casecmp(job->message->method, "PUT")) {
		handlerPutUser(job);
	} else if (StringUtils::casecmp(job->message->method, "DELETE")) {
		handlerDeleteUser(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
		                        SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->replyIsPrepared = true;
	}
}

static void addUserRolesMap(
  FaceRest::RestJob *job, JsonBuilderAgent &agent)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	UserRoleInfoList userRoleList;
	UserRoleQueryOption option(job->userId);
	dataStore->getUserRoleList(userRoleList, option);

	agent.startObject("userRoles");
	UserRoleInfoListIterator it = userRoleList.begin();
	agent.startObject(StringUtils::toString(NONE_PRIVILEGE));
	agent.add("name", "Guest");
	agent.endObject();
	agent.startObject(StringUtils::toString(ALL_PRIVILEGES));
	agent.add("name", "Admin");
	agent.endObject();
	for (; it != userRoleList.end(); ++it) {
		UserRoleInfo &userRoleInfo = *it;
		agent.startObject(StringUtils::toString(userRoleInfo.flags));
		agent.add("name", userRoleInfo.name);
		agent.endObject();
	}
	agent.endObject();
}

void FaceRest::handlerGetUser(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	UserInfoList userList;
	UserQueryOption option(job->userId);
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
	addUserRolesMap(job, agent);
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerPostUser(RestJob *job)
{
	UserInfo userInfo;
	HatoholError err = parseUserParameter(userInfo, job->query);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// try to add
	OperationPrivilege privilege(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->addUser(userInfo, privilege);

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", userInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerPutUser(RestJob *job)
{
	UserInfo userInfo;
	userInfo.id = job->getResourceId();
	if (userInfo.id == INVALID_USER_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", job->getResourceIdString().c_str());
		return;
	}

	DBClientUser dbUser;
	bool exist = dbUser.getUserInfo(userInfo, userInfo.id);
	if (!exist) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_USER_ID,
		            "id: %"FMT_USER_ID, userInfo.id);
		return;
	}
	bool forUpdate = true;
	HatoholError err = parseUserParameter(userInfo, job->query, forUpdate);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// try to update
	OperationPrivilege privilege(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->updateUser(userInfo, privilege);

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
	uint64_t userId = job->getResourceId();
	if (userId == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", job->getResourceIdString().c_str());
		return;
	}

	OperationPrivilege privilege(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->deleteUser(userId, privilege);

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", userId);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerAccessInfo(RestJob *job)
{
	if (StringUtils::casecmp(job->message->method, "GET")) {
		handlerGetAccessInfo(job);
	} else if (StringUtils::casecmp(job->message->method, "POST")) {
		handlerPostAccessInfo(job);
	} else if (StringUtils::casecmp(job->message->method, "DELETE")) {
		handlerDeleteAccessInfo(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
		                        SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->replyIsPrepared = true;
	}
}

void FaceRest::handlerGetAccessInfo(RestJob *job)
{
	AccessInfoQueryOption option(job->userId);
	option.setTargetUserId(job->getResourceId());

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerAccessInfoMap serversMap;
	HatoholError error = dataStore->getAccessInfoMap(serversMap, option);
	if (error != HTERR_OK) {
		replyError(job, error);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	ServerAccessInfoMapIterator it = serversMap.begin();
	agent.startObject("allowedServers");
	for (; it != serversMap.end(); it++) {
		uint32_t serverId = it->first;
		string serverIdString;
		HostGrpAccessInfoMap *hostGroupsMap = it->second;
		if (serverId == ALL_SERVERS)
			serverIdString = StringUtils::toString(-1);
		else
			serverIdString = StringUtils::toString(serverId);
		agent.startObject(serverIdString);
		agent.startObject("allowedHostGroups");
		HostGrpAccessInfoMapIterator it2 = hostGroupsMap->begin();
		for (; it2 != hostGroupsMap->end(); it2++) {
			AccessInfo *info = it2->second;
			uint64_t hostGroupId = it2->first;
			string hostGroupIdString;
			if (hostGroupId == ALL_HOST_GROUPS) {
				hostGroupIdString = StringUtils::toString(-1);
			} else {
				hostGroupIdString
				  = StringUtils::sprintf("%"PRIu64,
				                         hostGroupId);
			}
			agent.startObject(hostGroupIdString);
			agent.add("accessInfoId", info->id);
			agent.endObject();
		}
		agent.endObject();
		agent.endObject();
	}
	agent.endObject();
	agent.endObject();

	DBClientUser::destroyServerAccessInfoMap(serversMap);

	replyJsonData(agent, job);
}

void FaceRest::handlerPostAccessInfo(RestJob *job)
{
	// Get query parameters
	bool exist;
	bool succeeded;
	AccessInfo accessInfo;

	// userId
	int userIdPos = 0;
	accessInfo.userId = job->getResourceId(userIdPos);

	// serverId
	succeeded = getParamWithErrorReply<uint32_t>(
	              job, "serverId", "%"PRIu32, accessInfo.serverId, &exist);
	if (!succeeded)
		return;
	if (!exist) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "serverId");
		return;
	}

	// hostGroupId
	succeeded = getParamWithErrorReply<uint64_t>(
	              job, "hostGroupId", "%"PRIu64, accessInfo.hostGroupId, &exist);
	if (!succeeded)
		return;
	if (!exist) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_PARAMETER, "hostGroupId");
		return;
	}

	// try to add
	OperationPrivilege privilege(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->addAccessInfo(accessInfo, privilege);

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", accessInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerDeleteAccessInfo(RestJob *job)
{
	int nest = 1;
	uint64_t id = job->getResourceId(nest);
	if (id == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
			    "id: %s", job->getResourceIdString(nest).c_str());
		return;
	}

	OperationPrivilege privilege(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->deleteAccessInfo(id, privilege);

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerGetHostgroup(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	HostgroupInfoList hostgroupInfoList;
	HostgroupsQueryOption option(job->userId);
	HatoholError err =
	        dataStore->getHostgroupInfoList(hostgroupInfoList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.startArray("hostgroups");
	HostgroupInfoListIterator it = hostgroupInfoList.begin();
	for (; it != hostgroupInfoList.end(); ++it) {
		HostgroupInfo hostgroupInfo = *it;
		agent.startObject();
		agent.add("id", hostgroupInfo.id);
		agent.add("serverId", hostgroupInfo.serverId);
		agent.add("groupId", hostgroupInfo.groupId);
		agent.add("groupName", hostgroupInfo.groupName.c_str());
		addHostsIsMemberOfGroup(job, agent,
		                        hostgroupInfo.serverId,
		                        hostgroupInfo.groupId);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::addHostsIsMemberOfGroup(
  RestJob *job, JsonBuilderAgent &agent,
  uint64_t targetServerId, uint64_t targetGroupId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	HostgroupElementList hostgroupElementList;
	HostgroupElementQueryOption option(job->userId);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(targetGroupId);
	dataStore->getHostgroupElementList(hostgroupElementList, option);

	agent.startArray("hosts");
	HostgroupElementListIterator it = hostgroupElementList.begin();
	for (; it != hostgroupElementList.end(); ++it) {
		HostgroupElement hostgroupElement = *it;
		agent.add(hostgroupElement.hostId);
	}
	agent.endArray();
}

void FaceRest::handlerUserRole(RestJob *job)
{
	if (StringUtils::casecmp(job->message->method, "GET")) {
		handlerGetUserRole(job);
	} else if (StringUtils::casecmp(job->message->method, "POST")) {
		handlerPostUserRole(job);
	} else if (StringUtils::casecmp(job->message->method, "PUT")) {
		handlerPutUserRole(job);
	} else if (StringUtils::casecmp(job->message->method, "DELETE")) {
		handlerDeleteUserRole(job);
	} else {
		MLPL_ERR("Unknown method: %s\n", job->message->method);
		soup_message_set_status(job->message,
		                        SOUP_STATUS_METHOD_NOT_ALLOWED);
		job->replyIsPrepared = true;
	}
}

void FaceRest::handlerPutUserRole(RestJob *job)
{
	uint64_t id = job->getResourceId();
	if (id == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", job->getResourceIdString().c_str());
		return;
	}
	UserRoleInfo userRoleInfo;
	userRoleInfo.id = id;

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	UserRoleInfoList userRoleList;
	UserRoleQueryOption option(job->userId);
	option.setTargetUserRoleId(userRoleInfo.id);
	dataStore->getUserRoleList(userRoleList, option);
	if (userRoleList.empty()) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_USER_ROLE_ID,
		            "id: %"FMT_USER_ID, userRoleInfo.id);
		return;
	}
	userRoleInfo = *(userRoleList.begin());

	bool forUpdate = true;
	HatoholError err = parseUserRoleParameter(userRoleInfo, job->query,
						  forUpdate);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// try to update
	OperationPrivilege privilege(job->userId);
	err = dataStore->updateUserRole(userRoleInfo, privilege);

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", userRoleInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerGetUserRole(RestJob *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	UserRoleInfoList userRoleList;
	UserRoleQueryOption option(job->userId);
	dataStore->getUserRoleList(userRoleList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfUserRoles", userRoleList.size());
	agent.startArray("userRoles");
	UserRoleInfoListIterator it = userRoleList.begin();
	for (; it != userRoleList.end(); ++it) {
		const UserRoleInfo &userRoleInfo = *it;
		agent.startObject();
		agent.add("userRoleId",  userRoleInfo.id);
		agent.add("name", userRoleInfo.name);
		agent.add("flags", userRoleInfo.flags);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	replyJsonData(agent, job);
}

void FaceRest::handlerPostUserRole(RestJob *job)
{
	UserRoleInfo userRoleInfo;
	HatoholError err = parseUserRoleParameter(userRoleInfo, job->query);
	if (err != HTERR_OK) {
		replyError(job, err);
		return;
	}

	// try to add
	OperationPrivilege privilege(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->addUserRole(userRoleInfo, privilege);

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", userRoleInfo.id);
	agent.endObject();
	replyJsonData(agent, job);
}

void FaceRest::handlerDeleteUserRole(RestJob *job)
{
	uint64_t id = job->getResourceId();
	if (id == INVALID_ID) {
		REPLY_ERROR(job, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", job->getResourceIdString().c_str());
		return;
	}
	UserRoleIdType userRoleId = id;

	OperationPrivilege privilege(job->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->deleteUserRole(userRoleId, privilege);

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", userRoleId);
	agent.endObject();
	replyJsonData(agent, job);
}

HatoholError FaceRest::parseUserParameter(UserInfo &userInfo, GHashTable *query,
					  bool forUpdate)
{
	char *value;

	// name
	value = (char *)g_hash_table_lookup(query, "user");
	if (!value && !forUpdate)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "user\n");
	if (value)
		userInfo.name = value;

	// password
	value = (char *)g_hash_table_lookup(query, "password");
	if (!value && !forUpdate)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "password\n");
	userInfo.password = value ? value : "";

	// flags
	HatoholError err = getParam<OperationPrivilegeFlag>(
		query, "flags", "%"FMT_OPPRVLG, userInfo.flags);
	if (err != HTERR_OK && !forUpdate)
		return err;
	return HatoholError(HTERR_OK);
}

HatoholError FaceRest::parseUserRoleParameter(
  UserRoleInfo &userRoleInfo, GHashTable *query, bool forUpdate)
{
	char *value;

	// name
	value = (char *)g_hash_table_lookup(query, "name");
	if (!value && !forUpdate)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, "name\n");
	if (value)
		userRoleInfo.name = value;

	// flags
	HatoholError err = getParam<OperationPrivilegeFlag>(
		query, "flags", "%"FMT_OPPRVLG, userRoleInfo.flags);
	if (err != HTERR_OK && !forUpdate)
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

HatoholError FaceRest::parseEventParameter(EventsQueryOption &option,
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

