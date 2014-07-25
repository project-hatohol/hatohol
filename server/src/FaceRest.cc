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
#include <Mutex.h>
#include <SmartTime.h>
#include <AtomicValue.h>
#include <errno.h>
#include <uuid/uuid.h>
#include <semaphore.h>
#include "FaceRest.h"
#include "FaceRestPrivate.h"
#include "JSONBuilderAgent.h"
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
#include "RestResourceIssueTracker.h"
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
	static Mutex        lock;
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
	Mutex            restJobLock;
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
Mutex        FaceRest::PrivateContext::lock;

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
			job->handleInTryBlock();
			job->unpauseResponse();
			job->unref();
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
	RestResourceUser::registerFactories(this);
	RestResourceServer::registerFactories(this);
	RestResourceHost::registerFactories(this);
	RestResourceAction::registerFactories(this);
	RestResourceIssueTracker::registerFactories(this);

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
		job->handleInTryBlock();
		job->unpauseResponse();
		job->unref();
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
	soup_message_body_append(job->m_message->response_body,
				 SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(job->m_message, SOUP_STATUS_OK);
	job->m_replyIsPrepared = true;
}

void FaceRest::handlerTest(ResourceHandler *job)
{
	JSONBuilderAgent agent;
	agent.startObject();
	FaceRest::ResourceHandler::addHatoholError(
	  agent, HatoholError(HTERR_OK));
	if (PrivateContext::testMode)
		agent.addTrue("testMode");
	else
		agent.addFalse("testMode");

	if (string(job->m_path) == "/test" &&
	    string(job->m_message->method) == "POST")
	{
		agent.startObject("queryData");
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init(&iter, job->m_query);
		while (g_hash_table_iter_next(&iter, &key, &value)) {
			agent.add(string((const char *)key),
			          string((const char *)value));
		}
		agent.endObject(); // queryData
		agent.endObject(); // top level
		job->replyJSONData(agent);
		return;
	}

	if (string(job->m_path) == "/test/error") {
		agent.endObject(); // top level
		job->replyError(HTERR_ERROR_TEST);
		return;
	}

	if (string(job->m_path) == "/test/user" &&
	    string(job->m_message->method) == "POST")
	{
		RETURN_IF_NOT_TEST_MODE(job);
		UserQueryOption option(USER_ID_SYSTEM);
		HatoholError err
		  = RestResourceUser::updateOrAddUser(job->m_query, option);
		if (err != HTERR_OK) {
			job->replyError(err);
			return;
		}
	}

	agent.endObject();
	job->replyJSONData(agent);
}

void FaceRest::handlerLogin(ResourceHandler *job)
{
	gchar *user = (gchar *)g_hash_table_lookup(job->m_query, "user");
	if (!user) {
		MLPL_INFO("Not found: user\n");
		job->replyError(HTERR_AUTH_FAILED);
		return;
	}
	gchar *password
	  = (gchar *)g_hash_table_lookup(job->m_query, "password");
	if (!password) {
		MLPL_INFO("Not found: password\n");
		job->replyError(HTERR_AUTH_FAILED);
		return;
	}

	DBClientUser dbUser;
	UserIdType userId = dbUser.getUserId(user, password);
	if (userId == INVALID_USER_ID) {
		MLPL_INFO("Failed to authenticate: Client: %s, User: %s.\n",
			  soup_client_context_get_host(job->m_client), user);
		job->replyError(HTERR_AUTH_FAILED);
		return;
	}

	SessionManager *sessionMgr = SessionManager::getInstance();
	string sessionId = sessionMgr->create(userId);

	JSONBuilderAgent agent;
	agent.startObject();
	FaceRest::ResourceHandler::addHatoholError(
	  agent, HatoholError(HTERR_OK));
	agent.add("sessionId", sessionId);
	agent.endObject();

	job->replyJSONData(agent);
}

void FaceRest::handlerLogout(ResourceHandler *job)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	if (!sessionMgr->remove(job->m_sessionId)) {
		job->replyError(HTERR_NOT_FOUND_SESSION_ID);
		return;
	}

	JSONBuilderAgent agent;
	agent.startObject();
	FaceRest::ResourceHandler::addHatoholError(
	  agent, HatoholError(HTERR_OK));
	agent.endObject();

	job->replyJSONData(agent);
}


// ---------------------------------------------------------------------------
// FaceRest::ResourceHandler
// ---------------------------------------------------------------------------

FaceRest::ResourceHandler::ResourceHandler(FaceRest *faceRest,
					   RestHandlerFunc handler)
: m_faceRest(faceRest), m_staticHandlerFunc(handler), m_message(NULL),
  m_path(), m_query(NULL), m_client(NULL), m_mimeType(NULL),
  m_userId(INVALID_USER_ID), m_replyIsPrepared(false)
{
}

FaceRest::ResourceHandler::~ResourceHandler()
{
	if (m_query)
		g_hash_table_unref(m_query);
}

bool FaceRest::ResourceHandler::setRequest(
  SoupMessage *msg, const char *path, GHashTable *query,
  SoupClientContext *client)
{
	m_message  = msg;
	m_path     = path ? path : "";
	m_query    = query;
	m_client   = client;

	if (m_query)
		g_hash_table_ref(m_query);

	// Since life-span of other libsoup's objects should always be longer
	// than this object and shoube be managed by libsoup, we don't
	// inclement reference count of them.

	return parseRequest();
}

void FaceRest::ResourceHandler::handle(void)
{
	HATOHOL_ASSERT(m_staticHandlerFunc, "No handler function!");
	m_staticHandlerFunc(this);
}

void FaceRest::ResourceHandler::handleInTryBlock(void)
{
	try {
		handle();
	} catch (const HatoholException &e) {
		REPLY_ERROR(this, HTERR_GOT_EXCEPTION,
		            "%s", e.getFancyMessage().c_str());
	}
}

SoupServer *FaceRest::ResourceHandler::getSoupServer(void)
{
	return m_faceRest ? m_faceRest->getSoupServer() : NULL;
}

GMainContext *FaceRest::ResourceHandler::getGMainContext(void)
{
	return m_faceRest ? m_faceRest->getGMainContext() : NULL;
}

string FaceRest::ResourceHandler::getJSONPCallbackName(void)
{
	if (m_formatType != FORMAT_JSONP)
		return "";
	gpointer value = g_hash_table_lookup(m_query, "callback");
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
	m_formatString.clear();
	if (!m_query) {
		m_formatType = FORMAT_JSON;
		return true;
	}

	gchar *format = (gchar *)g_hash_table_lookup(m_query, "fmt");
	if (!format) {
		m_formatType = FORMAT_JSON; // default value
		return true;
	}
	m_formatString = format;

	FormatTypeMapIterator fmtIt = g_formatTypeMap.find(format);
	if (fmtIt == g_formatTypeMap.end())
		return false;
	m_formatType = fmtIt->second;
	return true;
}

bool FaceRest::ResourceHandler::parseRequest(void)
{
	const char *_sessionId =
	   soup_message_headers_get_one(m_message->request_headers,
	                                SESSION_ID_HEADER_NAME);
	m_sessionId = _sessionId ? _sessionId : "";

	bool notFoundSessionId = true;
	if (m_sessionId.empty()) {
		if (m_path == pathForLogin ||
		    PrivateContext::isTestPath(m_path)) {
			m_userId = INVALID_USER_ID;
			notFoundSessionId = false;
		}
	} else {
		SessionManager *sessionMgr = SessionManager::getInstance();
		SessionPtr session = sessionMgr->getSession(m_sessionId);
		if (session.hasData()) {
			notFoundSessionId = false;
			m_userId = session->userId;
		}
	}
	if (notFoundSessionId) {
		replyError(HTERR_NOT_FOUND_SESSION_ID);
		return false;
	}
	m_dataQueryContextPtr =
	  DataQueryContextPtr(new DataQueryContext(m_userId), false);

	// We expect URIs  whose style are the following.
	//
	// Examples:
	// http://localhost:33194/action
	// http://localhost:33194/action?fmt=json
	// http://localhost:33194/action/2345?fmt=html

	// a format type
	if (!parseFormatType()) {
		REPLY_ERROR(this, HTERR_UNSUPPORTED_FORMAT,
		            "%s", m_formatString.c_str());
		return false;
	}

	// path elements
	StringUtils::split(m_pathElements, m_path, '/');

	// MIME
	MimeTypeMapIterator mimeIt = g_mimeTypeMap.find(m_formatType);
	HATOHOL_ASSERT(
	  mimeIt != g_mimeTypeMap.end(),
	  "Invalid formatType: %d, %s", m_formatType, m_path.c_str());
	m_mimeType = mimeIt->second;

	// JSONP callback name
	m_jsonpCallbackName = getJSONPCallbackName();

	return true;
}

void FaceRest::ResourceHandler::pauseResponse(void)
{
	soup_server_pause_message(getSoupServer(), m_message);
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

bool FaceRest::ResourceHandler::unpauseResponse(bool force)
{
	if (!m_replyIsPrepared && !force)
		return false;

	if (g_main_context_acquire(getGMainContext())) {
		// FaceRest thread
		soup_server_unpause_message(getSoupServer(), m_message);
		g_main_context_release(getGMainContext());
	} else {
		// Other threads
		UnpauseContext *unpauseContext = new UnpauseContext;
		unpauseContext->server = getSoupServer();
		unpauseContext->message = m_message;
		soup_add_completion(getGMainContext(), idleUnpause,
				    unpauseContext);
	}

	return true;
}

bool FaceRest::ResourceHandler::httpMethodIs(const char *method)
{
	if (!m_message)
		return false;
	return StringUtils::casecmp(m_message->method, method);
}

string FaceRest::ResourceHandler::getResourceName(int nest)
{
	size_t idx = nest * 2;
	if (m_pathElements.size() > idx)
		return m_pathElements[idx];
	return string();
}

string FaceRest::ResourceHandler::getResourceIdString(int nest)
{
	size_t idx = nest * 2 + 1;
	if (m_pathElements.size() > idx)
		return m_pathElements[idx];
	return string();
}

uint64_t FaceRest::ResourceHandler::getResourceId(int nest)
{
	size_t idx = nest * 2 + 1;
	if (m_pathElements.size() <= idx)
		return INVALID_ID;
	uint64_t id = INVALID_ID;
	if (sscanf(m_pathElements[idx].c_str(), "%" PRIu64, &id) != 1)
		return INVALID_ID;
	return id;
}

void FaceRest::ResourceHandler::replyError(const HatoholErrorCode &errorCode,
					   const string &optionMessage)
{
	HatoholError hatoholError(errorCode, optionMessage);
	replyError(hatoholError);
}

static string wrapForJSONP(const string &jsonBody,
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

	JSONBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, hatoholError);
	agent.endObject();
	string response = agent.generate();
	if (!m_jsonpCallbackName.empty())
		response = wrapForJSONP(response, m_jsonpCallbackName);
	soup_message_headers_set_content_type(m_message->response_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(m_message->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(m_message, SOUP_STATUS_OK);

	m_replyIsPrepared = true;
}

void FaceRest::ResourceHandler::replyHttpStatus(const guint &statusCode)
{
	soup_message_set_status(m_message, statusCode);
	m_replyIsPrepared = true;
}

void FaceRest::ResourceHandler::replyJSONData(JSONBuilderAgent &agent)
{
	string response = agent.generate();
	if (!m_jsonpCallbackName.empty())
		response = wrapForJSONP(response, m_jsonpCallbackName);
	soup_message_headers_set_content_type(m_message->response_headers,
	                                      m_mimeType, NULL);
	soup_message_body_append(m_message->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(m_message, SOUP_STATUS_OK);

	m_replyIsPrepared = true;
}

void FaceRest::ResourceHandler::addHatoholError(JSONBuilderAgent &agent,
						const HatoholError &err)
{
	agent.add("apiVersion", API_VERSION);
	agent.add("errorCode", err.getCode());
	if (err != HTERR_OK && !err.getMessage().empty())
		agent.add("errorMessage", err.getMessage());
	if (!err.getOptionMessage().empty())
		agent.add("optionMessages", err.getOptionMessage());
}

static void addHostsMap(
  FaceRest::ResourceHandler *job, JSONBuilderAgent &agent,
  const MonitoringServerInfo &serverInfo)
{
	HostgroupIdType targetHostgroupId = ALL_HOST_GROUPS;
	char *value = (char *)g_hash_table_lookup(job->m_query, "hostgroupId");
	if (value)
		sscanf(value, "%" FMT_HOST_GROUP_ID, &targetHostgroupId);

	HostInfoList hostList;
	HostsQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetServerId(serverInfo.id);
	option.setTargetHostgroupId(targetHostgroupId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getHostList(hostList, option);
	HostInfoListIterator it = hostList.begin();
	agent.startObject("hosts");
	for (; it != hostList.end(); ++it) {
		HostInfo &host = *it;
		agent.startObject(StringUtils::toString(host.id));
		agent.add("name", host.hostName);
		agent.endObject();
	}
	agent.endObject();
}

static string getTriggerBrief(
  FaceRest::ResourceHandler *job, const ServerIdType serverId, const TriggerIdType triggerId)
{
	string triggerBrief;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList triggerInfoList;
	TriggersQueryOption triggersQueryOption(job->m_dataQueryContextPtr);
	triggersQueryOption.setTargetServerId(serverId);
	triggersQueryOption.setTargetId(triggerId);
	dataStore->getTriggerList(triggerInfoList, triggersQueryOption);

	TriggerInfoListIterator it = triggerInfoList.begin();
	TriggerInfo &firstTriggerInfo = *it;
	TriggerIdType firstId = firstTriggerInfo.id;
	for (; it != triggerInfoList.end(); ++it) {
		TriggerInfo &triggerInfo = *it;
		if (firstId == triggerInfo.id) {
			triggerBrief = triggerInfo.brief;
		} else {
			MLPL_WARN("Failed to getTriggerInfo: "
			          "%" FMT_SERVER_ID ", %" FMT_TRIGGER_ID "\n",
			          serverId, triggerId);
		}
	}

	return triggerBrief;
}

static void addTriggersIdBriefHash(
  FaceRest::ResourceHandler *job,
  JSONBuilderAgent &agent, const MonitoringServerInfo &serverInfo,
  TriggerBriefMaps &triggerMaps, bool lookupTriggerBrief = false)
{
	TriggerBriefMaps::iterator server_it = triggerMaps.find(serverInfo.id);
	agent.startObject("triggers");
	ServerIdType serverId = server_it->first;
	TriggerBriefMap &triggers = server_it->second;
	TriggerBriefMap::iterator it = triggers.begin();
	for (; server_it != triggerMaps.end() && it != triggers.end(); ++it) {
		TriggerIdType triggerId = it->first;
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

HatoholError FaceRest::ResourceHandler::addHostgroupsMap(
  JSONBuilderAgent &outputJSON, const MonitoringServerInfo &serverInfo,
  HostgroupInfoList &hostgroupList)
{
	HostgroupsQueryOption option(m_dataQueryContextPtr);
	option.setTargetServerId(serverInfo.id);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->getHostgroupInfoList(hostgroupList,
	                                                   option);
	if (err != HTERR_OK) {
		MLPL_ERR("Error: %d, user ID: %" FMT_USER_ID ", "
		         "sv ID: %" FMT_SERVER_ID "\n",
		         err.getCode(), m_userId, serverInfo.id);
		return err;
	}

	HostgroupInfoListIterator it = hostgroupList.begin();
	for (; it != hostgroupList.end(); ++it) {
		const HostgroupInfo &hostgroupInfo = *it;
		outputJSON.startObject(
		  StringUtils::toString(hostgroupInfo.groupId));
		outputJSON.add("name", hostgroupInfo.groupName);
		outputJSON.endObject();
	}
	return HTERR_OK;
}

void FaceRest::ResourceHandler::addServersMap(
  JSONBuilderAgent &agent,
  TriggerBriefMaps *triggerMaps, bool lookupTriggerBrief)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(m_dataQueryContextPtr);
	dataStore->getTargetServers(monitoringServers, option);

	HatoholError err;
	agent.startObject("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (; it != monitoringServers.end(); ++it) {
		const MonitoringServerInfo &serverInfo = *it;
		agent.startObject(StringUtils::toString(serverInfo.id));
		agent.add("name", serverInfo.hostName);
		agent.add("type", serverInfo.type);
		agent.add("ipAddress", serverInfo.ipAddress);
		addHostsMap(this, agent, serverInfo);
		if (triggerMaps) {
			addTriggersIdBriefHash(this, agent, serverInfo,
					       *triggerMaps,
			                       lookupTriggerBrief);
		}
		agent.startObject("groups");
		// Even if the following function retrun an error,
		// We cannot do anything. The receiver (client) should handle
		// the returned empty or unperfect group information.
		HostgroupInfoList hostgroupList;
		addHostgroupsMap(agent, serverInfo, hostgroupList);
		agent.endObject(); // "gropus"
		agent.endObject(); // toString(serverInfo.id)
	}
	agent.endObject();
}

FaceRest::ResourceHandlerFactory::ResourceHandlerFactory(
  FaceRest *faceRest, RestHandlerFunc handler)
: m_faceRest(faceRest), m_staticHandlerFunc(handler)
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
	return new ResourceHandler(m_faceRest, m_staticHandlerFunc);
}
