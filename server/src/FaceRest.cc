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
#include "FaceRestPrivate.h"
#include "JsonBuilderAgent.h"
#include "HatoholException.h"
#include "UnifiedDataStore.h"
#include "DBClientUser.h"
#include "DBClientConfig.h"
#include "SessionManager.h"
#include "CacheServiceDBClient.h"
#include "HatoholArmPluginInterface.h"
#include "HatoholArmPluginGate.h"
#include "RestResourceAction.h"
#include "RestResourceHost.h"
#include "RestResourceServer.h"
#include "RestResourceUser.h"

using namespace std;
using namespace mlpl;

int FaceRest::API_VERSION = 3;
const char *FaceRest::SESSION_ID_HEADER_NAME = "X-Hatohol-Session";
const int FaceRest::DEFAULT_NUM_WORKERS = 4;

static const guint DEFAULT_PORT = 33194;

const char *FaceRest::pathForTest   = "/test";
const char *FaceRest::pathForLogin  = "/login";
const char *FaceRest::pathForLogout = "/logout";

static const char *MIME_HTML = "text/html";
static const char *MIME_JSON = "application/json";
static const char *MIME_JAVASCRIPT = "text/javascript";

#define RETURN_IF_NOT_TEST_MODE(JOB) \
do { \
	if (!isTestMode()) { \
		JOB->replyError(HTERR_NOT_TEST_MODE); \
		return; \
	}\
} while(0)

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
	bool             asyncMode;
	size_t           numPreLoadWorkers;
	set<Worker *>    workers;
	queue<ResourceHandler *> restJobQueue;
	MutexLock        restJobLock;
	sem_t            waitJobSemaphore;

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

	~PrivateContext()
	{
		g_main_context_unref(gMainCtx);
		sem_destroy(&waitJobSemaphore);
	}

	static string initPathForUserMe(void)
	{
		return string(RestResourceUser::pathForUser) + "/me";
	}

	void pushJob(ResourceHandler *job)
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

	ResourceHandler *popJob(void)
	{
		ResourceHandler *job = NULL;
		restJobLock.lock();
		if (!restJobQueue.empty()) {
			job = restJobQueue.front();
			restJobQueue.pop();
		}
		restJobLock.unlock();
		return job;
	}

	void addHandler(const char *path, ResourceHandlerFactory *factory)
	{
		soup_server_add_handler(soupServer, path,
					queueRestJob, factory,
					ResourceHandlerFactory::destroy);
	}

	static void queueRestJob
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);

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

class FaceRest::Worker : public HatoholThreadBase {
public:
	Worker(FaceRest *faceRest)
	: m_faceRest(faceRest)
	{
	}
	virtual ~Worker()
	{
	}

	virtual void waitExit(void) override
	{
		if (!isStarted())
			return;
		HatoholThreadBase::waitExit();
	}

protected:
	virtual gpointer mainThread(HatoholThreadArg *arg)
	{
		ResourceHandler *job;
		MLPL_INFO("start face-rest worker\n");
		while ((job = waitNextJob())) {
			launchHandlerInTryBlock(job);
			finishRestJobIfNeeded(job);
		}
		MLPL_INFO("exited face-rest worker\n");
		return NULL;
	}

private:
	ResourceHandler *waitNextJob(void)
	{
		while (m_faceRest->m_ctx->waitJob()) {
			ResourceHandler *job = m_faceRest->m_ctx->popJob();
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
	waitExit();

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

void FaceRest::waitExit(void)
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

	HatoholThreadBase::waitExit();
}

void FaceRest::setNumberOfPreLoadWorkers(size_t num)
{
	m_ctx->numPreLoadWorkers = num;
}

void FaceRest::addResourceHandlerFactory(const char *path,
					 ResourceHandlerFactory *factory)
{
	m_ctx->addHandler(path, factory);
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
	m_ctx->addHandler("/hello.html",
			  new ResourceHandlerFactory(this, &handlerHelloPage));
	m_ctx->addHandler(pathForTest,
			  new ResourceHandlerFactory(this, handlerTest));
	m_ctx->addHandler(pathForLogin,
			  new ResourceHandlerFactory(this, handlerLogin));
	m_ctx->addHandler(pathForLogout,
			  new ResourceHandlerFactory(this, handlerLogout));
	m_ctx->addHandler(RestResourceHost::pathForOverview,
			  new RestResourceHostFactory(
			    this, RestResourceHost::handlerGetOverview));
	m_ctx->addHandler(RestResourceServer::pathForServer,
			  new RestResourceServerFactory(
			    this, RestResourceServer::handlerServer));
	m_ctx->addHandler(RestResourceServer::pathForServerConnStat,
			  new RestResourceServerFactory(
			    this, RestResourceServer::handlerServerConnStat));
	m_ctx->addHandler(RestResourceHost::pathForHost,
			  new RestResourceHostFactory(
			    this, RestResourceHost::handlerGetHost));
	m_ctx->addHandler(RestResourceHost::pathForTrigger,
			  new RestResourceHostFactory(
			    this, RestResourceHost::handlerGetTrigger));
	m_ctx->addHandler(RestResourceHost::pathForEvent,
			  new RestResourceHostFactory(
			    this, RestResourceHost::handlerGetEvent));
	m_ctx->addHandler(RestResourceHost::pathForItem,
			  new RestResourceHostFactory(
			    this, RestResourceHost::handlerGetItem));
	m_ctx->addHandler(RestResourceAction::pathForAction,
			  new RestResourceActionFactory(
			    this, RestResourceAction::handlerAction));
	m_ctx->addHandler(RestResourceHost::pathForHostgroup,
			  new RestResourceHostFactory(
			    this, RestResourceHost::handlerGetHostgroup));
	m_ctx->addHandler(RestResourceUser::pathForUser,
			  new RestResourceUserFactory(
			    this, RestResourceUser::handlerUser));
	m_ctx->addHandler(RestResourceUser::pathForUserRole,
			  new RestResourceUserFactory(
			   this, RestResourceUser::handlerUserRole));
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

SoupServer *FaceRest::getSoupServer(void)
{
	return m_ctx->soupServer;
}

GMainContext *FaceRest::getGMainContext(void)
{
	return m_ctx->gMainCtx;
}

const std::string &FaceRest::getPathForUserMe(void)
{
	return m_ctx->pathForUserMe;
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

// handlers
void FaceRest::handlerDefault(SoupServer *server, SoupMessage *msg,
                              const char *path, GHashTable *query,
                              SoupClientContext *client, gpointer user_data)
{
	MLPL_DBG("Default handler: path: %s, method: %s\n",
	         path, msg->method);
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}

static void copyHashTable (gpointer key, gpointer data, gpointer user_data)
{
	GHashTable *dest = static_cast<GHashTable *>(user_data);
	g_hash_table_insert(dest,
			    g_strdup(static_cast<gchar*>(key)),
			    g_strdup(static_cast<gchar*>(data)));
}

void FaceRest::PrivateContext::queueRestJob
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

	ResourceHandlerFactory *factory
	  = static_cast<ResourceHandlerFactory *>(user_data);
	FaceRest *face = factory->m_faceRest;
	ResourceHandler *job = factory->createHandler();
	bool succeeded = job->setRequest(msg, path, query, client);
	if (!succeeded)
		return;

	job->pauseResponse();

	if (face->isAsyncMode()) {
		face->m_ctx->pushJob(job);
	} else {
		launchHandlerInTryBlock(job);
		finishRestJobIfNeeded(job);
	}
}

void FaceRest::finishRestJobIfNeeded(ResourceHandler *job)
{
	if (job->replyIsPrepared)
		job->unpauseResponse();
	job->unref();
}

void FaceRest::launchHandlerInTryBlock(ResourceHandler *job)
{
	try {
		job->handle();
	} catch (const HatoholException &e) {
		REPLY_ERROR(job, HTERR_GOT_EXCEPTION,
		            "%s", e.getFancyMessage().c_str());
	}
}

void FaceRest::handlerHelloPage(ResourceHandler *job)
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

void FaceRest::handlerTest(ResourceHandler *job)
{
	JsonBuilderAgent agent;
	agent.startObject();
	FaceRest::ResourceHandler::addHatoholError(
	  agent, HatoholError(HTERR_OK));
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
		job->replyJsonData(agent);
		return;
	}

	if (string(job->path) == "/test/error") {
		agent.endObject(); // top level
		job->replyError(HTERR_ERROR_TEST);
		return;
	}

	if (string(job->path) == "/test/user" &&
	    string(job->message->method) == "POST")
	{
		RETURN_IF_NOT_TEST_MODE(job);
		UserQueryOption option(USER_ID_SYSTEM);
		HatoholError err = updateOrAddUser(job->query, option);
		if (err != HTERR_OK) {
			job->replyError(err);
			return;
		}
	}

	agent.endObject();
	job->replyJsonData(agent);
}

void FaceRest::handlerLogin(ResourceHandler *job)
{
	gchar *user = (gchar *)g_hash_table_lookup(job->query, "user");
	if (!user) {
		MLPL_INFO("Not found: user\n");
		job->replyError(HTERR_AUTH_FAILED);
		return;
	}
	gchar *password = (gchar *)g_hash_table_lookup(job->query, "password");
	if (!password) {
		MLPL_INFO("Not found: password\n");
		job->replyError(HTERR_AUTH_FAILED);
		return;
	}

	DBClientUser dbUser;
	UserIdType userId = dbUser.getUserId(user, password);
	if (userId == INVALID_USER_ID) {
		MLPL_INFO("Failed to authenticate: Client: %s, User: %s.\n",
			  soup_client_context_get_host(job->client), user);
		job->replyError(HTERR_AUTH_FAILED);
		return;
	}

	SessionManager *sessionMgr = SessionManager::getInstance();
	string sessionId = sessionMgr->create(userId);

	JsonBuilderAgent agent;
	agent.startObject();
	FaceRest::ResourceHandler::addHatoholError(
	  agent, HatoholError(HTERR_OK));
	agent.add("sessionId", sessionId);
	agent.endObject();

	job->replyJsonData(agent);
}

void FaceRest::handlerLogout(ResourceHandler *job)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	if (!sessionMgr->remove(job->sessionId)) {
		job->replyError(HTERR_NOT_FOUND_SESSION_ID);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	FaceRest::ResourceHandler::addHatoholError(
	  agent, HatoholError(HTERR_OK));
	agent.endObject();

	job->replyJsonData(agent);
}


// ---------------------------------------------------------------------------
// FaceRest::ResourceHandler
// ---------------------------------------------------------------------------

FaceRest::ResourceHandler::ResourceHandler(FaceRest *_faceRest,
					   RestHandler _handler)
: message(NULL), path(), query(NULL), client(NULL),
  faceRest(_faceRest), handler(_handler), mimeType(NULL),
  userId(INVALID_USER_ID), replyIsPrepared(false)
{
}

bool FaceRest::ResourceHandler::setRequest(
  SoupMessage *_msg, const char *_path, GHashTable *_query,
  SoupClientContext *_client)
{
	message  = _msg;
	path     = _path ? _path : "";
	query    = _query;
	client   = _client;

	if (query)
		g_hash_table_ref(query);

	// Since life-span of other libsoup's objects should always be longer
	// than this object and shoube be managed by libsoup, we don't
	// inclement reference count of them.

	return prepare();
}

void FaceRest::ResourceHandler::handle(void)
{
	handler(this);
}

FaceRest::ResourceHandler::~ResourceHandler()
{
	if (query)
		g_hash_table_unref(query);
}

SoupServer *FaceRest::ResourceHandler::getSoupServer(void)
{
	return faceRest ? faceRest->getSoupServer() : NULL;
}

GMainContext *FaceRest::ResourceHandler::getGMainContext(void)
{
	return faceRest ? faceRest->getGMainContext() : NULL;
}

bool FaceRest::ResourceHandler::pathIsUserMe(void)
{
	if (!faceRest)
		return false;
	return (path == faceRest->getPathForUserMe());
}

string FaceRest::ResourceHandler::getJsonpCallbackName(void)
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

bool FaceRest::ResourceHandler::parseFormatType(void)
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

bool FaceRest::ResourceHandler::prepare(void)
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
		replyError(HTERR_NOT_FOUND_SESSION_ID);
		return false;
	}
	dataQueryContextPtr =
	  DataQueryContextPtr(new DataQueryContext(userId), false);

	// We expect URIs  whose style are the following.
	//
	// Examples:
	// http://localhost:33194/action
	// http://localhost:33194/action?fmt=json
	// http://localhost:33194/action/2345?fmt=html

	// a format type
	if (!parseFormatType()) {
		REPLY_ERROR(this, HTERR_UNSUPPORTED_FORMAT,
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

void FaceRest::ResourceHandler::pauseResponse(void)
{
	soup_server_pause_message(getSoupServer(), message);
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

void FaceRest::ResourceHandler::unpauseResponse(void)
{
	if (g_main_context_acquire(getGMainContext())) {
		// FaceRest thread
		soup_server_unpause_message(getSoupServer(), message);
		g_main_context_release(getGMainContext());
	} else {
		// Other threads
		UnpauseContext *unpauseContext = new UnpauseContext;
		unpauseContext->server = getSoupServer();
		unpauseContext->message = message;
		soup_add_completion(getGMainContext(), idleUnpause,
				    unpauseContext);
	}
}

string FaceRest::ResourceHandler::getResourceName(int nest)
{
	size_t idx = nest * 2;
	if (pathElements.size() > idx)
		return pathElements[idx];
	return string();
}

string FaceRest::ResourceHandler::getResourceIdString(int nest)
{
	size_t idx = nest * 2 + 1;
	if (pathElements.size() > idx)
		return pathElements[idx];
	return string();
}

uint64_t FaceRest::ResourceHandler::getResourceId(int nest)
{
	size_t idx = nest * 2 + 1;
	if (pathElements.size() <= idx)
		return INVALID_ID;
	uint64_t id = INVALID_ID;
	if (sscanf(pathElements[idx].c_str(), "%" PRIu64, &id) != 1)
		return INVALID_ID;
	return id;
}

void FaceRest::ResourceHandler::replyError(const HatoholErrorCode &errorCode,
					   const string &optionMessage)
{
	HatoholError hatoholError(errorCode, optionMessage);
	replyError(hatoholError);
}

static string wrapForJsonp(const string &jsonBody,
			   const string &callbackName)
{
	string jsonp = callbackName;
	jsonp += "(";
	jsonp += jsonBody;
	jsonp += ")";
	return jsonp;
}

void FaceRest::ResourceHandler::replyError(const HatoholError &hatoholError)
{
	string error
	  = StringUtils::sprintf("%d", hatoholError.getCode());
	const string &codeName = hatoholError.getCodeName();
	const string &optionMessage = hatoholError.getOptionMessage();

	if (!codeName.empty()) {
		error += ", ";
		error += codeName;
	}
	if (!optionMessage.empty()) {
		error += ": ";
		error += optionMessage;
	}
	MLPL_INFO("reply error: %s\n", error.c_str());

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, hatoholError);
	agent.endObject();
	string response = agent.generate();
	if (!jsonpCallbackName.empty())
		response = wrapForJsonp(response, jsonpCallbackName);
	soup_message_headers_set_content_type(message->response_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(message->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(message, SOUP_STATUS_OK);

	replyIsPrepared = true;
}

void FaceRest::ResourceHandler::replyJsonData(JsonBuilderAgent &agent)
{
	string response = agent.generate();
	if (!jsonpCallbackName.empty())
		response = wrapForJsonp(response, jsonpCallbackName);
	soup_message_headers_set_content_type(message->response_headers,
	                                      mimeType, NULL);
	soup_message_body_append(message->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(message, SOUP_STATUS_OK);

	replyIsPrepared = true;
}

void FaceRest::ResourceHandler::addHatoholError(JsonBuilderAgent &agent,
						const HatoholError &err)
{
	agent.add("apiVersion", API_VERSION);
	agent.add("errorCode", err.getCode());
	if (err != HTERR_OK && !err.getMessage().empty())
		agent.add("errorMessage", err.getMessage());
	if (!err.getOptionMessage().empty())
		agent.add("optionMessages", err.getOptionMessage());
}


FaceRest::ResourceHandlerFactory::ResourceHandlerFactory(
  FaceRest *faceRest, RestHandler handler)
: m_faceRest(faceRest), m_handler(handler)
{
}

FaceRest::ResourceHandlerFactory::~ResourceHandlerFactory()
{
}

void FaceRest::ResourceHandlerFactory::destroy(gpointer data)
{
	ResourceHandlerFactory *factory
		= static_cast<ResourceHandlerFactory *>(data);
	delete factory;
}

FaceRest::ResourceHandler *
FaceRest::ResourceHandlerFactory::createHandler(void)
{
	return new ResourceHandler(m_faceRest, m_handler);
}
