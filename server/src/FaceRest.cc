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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstring>
#include <queue>
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
#include "JSONBuilder.h"
#include "HatoholException.h"
#include "UnifiedDataStore.h"
#include "DBTablesUser.h"
#include "DBTablesConfig.h"
#include "SessionManager.h"
#include "ThreadLocalDBCache.h"
#include "RestResourceSystem.h"
#include "RestResourceAction.h"
#include "RestResourceMonitoring.h"
#include "RestResourceIncident.h"
#include "RestResourceIncidentTracker.h"
#include "RestResourceServer.h"
#include "RestResourceSeverityRank.h"
#include "RestResourceSummary.h"
#include "RestResourceCustomIncidentStatus.h"
#include "RestResourceUser.h"
#include "ConfigManager.h"

using namespace std;
using namespace mlpl;

int FaceRest::API_VERSION = 4;
const char *FaceRest::SESSION_ID_HEADER_NAME = "X-Hatohol-Session";
const int FaceRest::DEFAULT_NUM_WORKERS = 4;

static const guint DEFAULT_PORT = 33194;

const char *FaceRest::pathForTest   = "/test";
const char *FaceRest::pathForLogin  = "/login";
const char *FaceRest::pathForLogout = "/logout";

static const char *MIME_HTML = "text/html";
static const char *MIME_JSON = "application/json";
static const char *MIME_JAVASCRIPT = "text/javascript";

#define RETURN_IF_NOT_TEST_MODE(TEST_MODE, JOB) \
do { \
	if (!TEST_MODE) { \
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

// FaceRestPrivate ===========================================================
template<>
int scanParam(const char *value, const char *scanFmt, std::string &dest)
{
	if (!value)
		return 0;
	dest = value;
	return !dest.empty();
}

// FaceRest ==================================================================
struct FaceRest::Impl {
	struct MainThreadCleaner;
	static Mutex        lock;
	guint               port;
	SoupServer         *soupServer;
	GMainContext       *gMainCtx;
	FaceRestParam      *param;
	AtomicValue<bool>   quitRequest;
	set<string>         handlerPathSet;

	// for async mode
	bool             asyncMode;
	size_t           numPreLoadWorkers;
	set<Worker *>    workers;
	queue<ResourceHandler *> restJobQueue;
	Mutex            restJobLock;
	sem_t            waitJobSemaphore;

	Impl(FaceRestParam *_param)
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

	~Impl()
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
		handlerPathSet.insert(path);
	}

	void removeAllHandlers(void)
	{
		if (!soupServer)
			return;

		set<string>::iterator it = handlerPathSet.begin();
		for (; it != handlerPathSet.end(); it++)
			soup_server_remove_handler(soupServer, (*it).c_str());
		handlerPathSet.clear();
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

Mutex        FaceRest::Impl::lock;

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
		while (m_faceRest->m_impl->waitJob()) {
			ResourceHandler *job = m_faceRest->m_impl->popJob();
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

FaceRest::FaceRest(FaceRestParam *param)
: m_impl(new Impl(param))
{
	int port = ConfigManager::getInstance()->getFaceRestPort();
	if (port) {
		m_impl->port = port;
	} else {
		ThreadLocalDBCache cache;
		int port = cache.getConfig().getFaceRestPort();
		if (port != 0 && Utils::isValidPort(port))
			m_impl->port = port;
	}

	int num = ConfigManager::getInstance()->getFaceRestNumWorkers();
	if (num > 0) {
		setNumberOfPreLoadWorkers(num);
	}

	MLPL_INFO("started face-rest, port: %d, workers: %zu\n",
		  m_impl->port, m_impl->numPreLoadWorkers);
}

FaceRest::~FaceRest()
{
	waitExit();

	MLPL_INFO("FaceRest: stop process: started.\n");
	if (m_impl->soupServer) {
		SoupSocket *sock = soup_server_get_listener(m_impl->soupServer);
		soup_socket_disconnect(sock);
		m_impl->removeAllHandlers();
		g_object_unref(m_impl->soupServer);
	}
	MLPL_INFO("FaceRest: stop process: completed.\n");
}

void FaceRest::waitExit(void)
{
	if (isStarted()) {
		m_impl->quitRequest.set(true);

		// To return g_main_context_iteration() in mainThread()
		struct IterAlarm {
			static gboolean task(gpointer data) {
				return G_SOURCE_REMOVE;
			}
		};
		Utils::setGLibIdleEvent(IterAlarm::task, NULL, m_impl->gMainCtx);
	}

	HatoholThreadBase::waitExit();
}

void FaceRest::setNumberOfPreLoadWorkers(size_t num)
{
	m_impl->numPreLoadWorkers = num;
}

void FaceRest::addResourceHandlerFactory(const char *path,
					 ResourceHandlerFactory *factory)
{
	m_impl->addHandler(path, factory);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
struct FaceRest::Impl::MainThreadCleaner {
	Impl *impl;
	bool running;

	MainThreadCleaner(Impl *_impl)
	: impl(_impl),
	  running(false)
	{
	}

	static void callgate(MainThreadCleaner *obj)
	{
		obj->run();
	}

	void run(void)
	{
		if (impl->soupServer && running)
			soup_server_quit(impl->soupServer);
	}
};

bool FaceRest::isAsyncMode(void)
{
	return m_impl->asyncMode;
}

void FaceRest::startWorkers(void)
{
	for (size_t i = 0; i < m_impl->numPreLoadWorkers; i++) {
		Worker *worker = new Worker(this);
		worker->start();
		m_impl->workers.insert(worker);
	}
}

void FaceRest::stopWorkers(void)
{
	set<Worker *> &workers = m_impl->workers;
	set<Worker *>::iterator it;
	for (it = workers.begin(); it != workers.end(); ++it) {
		// to break Worker::waitNextJob()
		sem_post(&m_impl->waitJobSemaphore);
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
	Impl::MainThreadCleaner cleaner(m_impl.get());
	Reaper<Impl::MainThreadCleaner>
	   reaper(&cleaner, Impl::MainThreadCleaner::callgate);

	m_impl->soupServer = soup_server_new(SOUP_SERVER_PORT, m_impl->port,
	                                    SOUP_SERVER_ASYNC_CONTEXT,
	                                    m_impl->gMainCtx, NULL);
	if (errno == EADDRINUSE)
		MLPL_ERR("%s", Utils::getUsingPortInfo(m_impl->port).c_str());
	HATOHOL_ASSERT(m_impl->soupServer,
	               "failed: soup_server_new: %u, errno: %d (%s)\n",
	               m_impl->port, errno, g_strerror(errno));
	soup_server_add_handler(m_impl->soupServer, NULL,
	                        handlerDefault, this, NULL);
	m_impl->addHandler(
	  "/hello.html",
	  new RestResourceStaticHandlerFactory(this, &handlerHelloPage));
	m_impl->addHandler(
	  pathForTest,
	  new RestResourceStaticHandlerFactory(this, handlerTest));
	m_impl->addHandler(
	  pathForLogin,
	  new RestResourceStaticHandlerFactory(this, handlerLogin));
	m_impl->addHandler(
	  pathForLogout,
	  new RestResourceStaticHandlerFactory(this, handlerLogout));
	RestResourceSystem::registerFactories(this);
	RestResourceUser::registerFactories(this);
	RestResourceServer::registerFactories(this);
	RestResourceMonitoring::registerFactories(this);
	RestResourceIncident::registerFactories(this);
	RestResourceAction::registerFactories(this);
	RestResourceIncidentTracker::registerFactories(this);
	RestResourceSeverityRank::registerFactories(this);
	RestResourceSummary::registerFactories(this);
	RestResourceCustomIncidentStatus::registerFactories(this);

	if (m_impl->param)
		m_impl->param->setupDoneNotifyFunc();
	soup_server_run_async(m_impl->soupServer);
	cleaner.running = true;

	if (isAsyncMode())
		startWorkers();

	while (!m_impl->quitRequest.get())
		g_main_context_iteration(m_impl->gMainCtx, TRUE);

	if (isAsyncMode())
		stopWorkers();

	MLPL_INFO("exited face-rest\n");
	return NULL;
}

SoupServer *FaceRest::getSoupServer(void)
{
	return m_impl->soupServer;
}

GMainContext *FaceRest::getGMainContext(void)
{
	return m_impl->gMainCtx;
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

void FaceRest::Impl::queueRestJob
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
	if (!succeeded) {
		job->unref();
		return;
	}

	job->pauseResponse();

	if (face->isAsyncMode()) {
		face->m_impl->pushJob(job);
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
	JSONBuilder agent;
	agent.startObject();
	FaceRest::ResourceHandler::addHatoholError(
	  agent, HatoholError(HTERR_OK));
	const bool testMode = ConfigManager::getInstance()->isTestMode();
	if (testMode)
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
		RETURN_IF_NOT_TEST_MODE(testMode, job);
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

	ThreadLocalDBCache cache;
	UserIdType userId = cache.getUser().getUserId(user, password);
	if (userId == INVALID_USER_ID) {
		MLPL_INFO("Failed to authenticate: Client: %s, User: %s.\n",
			  soup_client_context_get_host(job->m_client), user);
		job->replyError(HTERR_AUTH_FAILED);
		return;
	}

	SessionManager *sessionMgr = SessionManager::getInstance();
	string sessionId = sessionMgr->create(userId);

	JSONBuilder agent;
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

	JSONBuilder agent;
	agent.startObject();
	FaceRest::ResourceHandler::addHatoholError(
	  agent, HatoholError(HTERR_OK));
	agent.endObject();

	job->replyJSONData(agent);
}


// ---------------------------------------------------------------------------
// FaceRest::ResourceHandler
// ---------------------------------------------------------------------------
FaceRest::ResourceHandler::ResourceHandler(FaceRest *faceRest)
: m_faceRest(faceRest), m_message(NULL),
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
		    Impl::isTestPath(m_path)) {
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
	UnpauseContext *impl = static_cast<UnpauseContext *>(data);
	soup_server_unpause_message(impl->server,
				    impl->message);
	delete impl;
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
					   const string &optionMessage,
					   const guint &statusCode)
{
	HatoholError hatoholError(errorCode, optionMessage);
	replyError(hatoholError, statusCode);
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

void FaceRest::ResourceHandler::replyError(const HatoholError &hatoholError,
					   const guint &statusCode)
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

	JSONBuilder agent;
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
	soup_message_set_status(m_message, statusCode);

	m_replyIsPrepared = true;
}

void FaceRest::ResourceHandler::replyHttpStatus(const guint &statusCode)
{
	soup_message_set_status(m_message, statusCode);
	m_replyIsPrepared = true;
}

void FaceRest::ResourceHandler::replyJSONData(JSONBuilder &agent,
					      const guint &statusCode)
{
	string response = agent.generate();
	if (!m_jsonpCallbackName.empty())
		response = wrapForJSONP(response, m_jsonpCallbackName);
	soup_message_headers_set_content_type(m_message->response_headers,
	                                      m_mimeType, NULL);
	soup_message_body_append(m_message->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(m_message, statusCode);

	m_replyIsPrepared = true;
}

void FaceRest::ResourceHandler::addHatoholError(JSONBuilder &agent,
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
  FaceRest::ResourceHandler *job, JSONBuilder &agent,
  const MonitoringServerInfo &serverInfo)
{
	HostgroupIdType targetHostgroupId = ALL_HOST_GROUPS;
	char *value = (char *)g_hash_table_lookup(job->m_query, "hostgroupId");
	if (value)
		targetHostgroupId = value;

	ServerHostDefVect svHostDefVect;
	HostsQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetServerId(serverInfo.id);
	option.setTargetHostgroupId(targetHostgroupId);
	// TODO:
	// This is a workaround to show host name that was deleted in the
	// Event page. We should save host name in Event table.
	option.setStatusSet({HOST_STAT_NORMAL, HOST_STAT_SELF_MONITOR});

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
	  dataStore->getServerHostDefs(svHostDefVect, option));
	ServerHostDefVectConstIterator svHostIt = svHostDefVect.begin();
	agent.startObject("hosts");
	for (; svHostIt != svHostDefVect.end(); ++svHostIt) {
		const ServerHostDef &svHostDef = *svHostIt;
		agent.startObject(svHostDef.hostIdInServer);
		agent.add("name", svHostDef.name);
		agent.endObject();
	}
	agent.endObject();
}

static string getTriggerBrief(
  FaceRest::ResourceHandler *job, const ServerIdType &serverId,
  const TriggerIdType &triggerId)
{
	string triggerBrief;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList triggerInfoList;
	TriggersQueryOption triggersQueryOption(job->m_dataQueryContextPtr);
	triggersQueryOption.setTargetServerId(serverId);
	triggersQueryOption.setTargetId(triggerId);
	dataStore->getTriggerList(triggerInfoList, triggersQueryOption);
	if (triggerInfoList.empty()) {
		MLPL_WARN("Failed to getTriggerInfo (Not found trigger): "
		          "%" FMT_SERVER_ID ", %" FMT_TRIGGER_ID "\n",
		          serverId, triggerId.c_str());
		return "";
	}

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
			          serverId, triggerId.c_str());
		}
	}

	return triggerBrief;
}

static void addTriggersIdBriefHash(
  FaceRest::ResourceHandler *job,
  JSONBuilder &agent, const MonitoringServerInfo &serverInfo,
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
		agent.startObject(triggerId);
		agent.add("brief", triggerBrief);
		agent.endObject();
	}
	agent.endObject();
}

HatoholError FaceRest::ResourceHandler::addHostgroupsMap(
  JSONBuilder &outputJSON, const MonitoringServerInfo &serverInfo,
  HostgroupVect &hostgroups)
{
	HostgroupsQueryOption option(m_dataQueryContextPtr);
	option.setTargetServerId(serverInfo.id);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->getHostgroups(hostgroups, option);
	if (err != HTERR_OK) {
		MLPL_ERR("Error: %d, user ID: %" FMT_USER_ID ", "
		         "sv ID: %" FMT_SERVER_ID "\n",
		         err.getCode(), m_userId, serverInfo.id);
		return err;
	}

	HostgroupVectConstIterator it = hostgroups.begin();
	for (; it != hostgroups.end(); ++it) {
		const Hostgroup &hostgrp = *it;
		outputJSON.startObject(hostgrp.idInServer);
		outputJSON.add("name", hostgrp.name);
		outputJSON.endObject();
	}
	return HTERR_OK;
}

void FaceRest::ResourceHandler::addServersMap(
  JSONBuilder &agent,
  TriggerBriefMaps *triggerMaps, bool lookupTriggerBrief)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(m_dataQueryContextPtr);
	ArmPluginInfoVect armPluginInfoVect;
	dataStore->getTargetServers(monitoringServers, option,
	                            &armPluginInfoVect);

	HatoholError err;
	agent.startObject("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	ArmPluginInfoVectConstIterator pluginIt = armPluginInfoVect.begin();
	HATOHOL_ASSERT(monitoringServers.size() == armPluginInfoVect.size(),
	               "The number of elements differs: %zd, %zd",
	               monitoringServers.size(), armPluginInfoVect.size());
	for (; it != monitoringServers.end(); ++it, ++pluginIt) {
		const MonitoringServerInfo &serverInfo = *it;
		agent.startObject(to_string(serverInfo.id));
		agent.add("name", serverInfo.hostName);
		agent.add("nickname", serverInfo.nickname);
		agent.add("type", serverInfo.type);
		agent.add("ipAddress", serverInfo.ipAddress);
		agent.add("baseURL", serverInfo.baseURL);
		if (pluginIt->id != INVALID_ARM_PLUGIN_INFO_ID) {
			agent.add("uuid", pluginIt->uuid);
		}
		addHostsMap(this, agent, serverInfo);
		if (triggerMaps) {
			addTriggersIdBriefHash(this, agent, serverInfo,
					       *triggerMaps,
			                       lookupTriggerBrief);
		}
		agent.startObject("groups");
		HostgroupVect hostgroups;
		THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
		  addHostgroupsMap(agent, serverInfo, hostgroups));
		agent.endObject(); // "gropus"
		agent.endObject(); // to_string(serverInfo.id)
	}
	agent.endObject();
}

void FaceRest::ResourceHandler::addIncidentTrackersMap(JSONBuilder &agent)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	IncidentTrackerInfoVect trackers;
	IncidentTrackerQueryOption option(m_dataQueryContextPtr);
	dataStore->getIncidentTrackers(trackers, option);

	HatoholError err;
	agent.startObject("incidentTrackers");
	for (auto &tracker : trackers) {
		agent.startObject(to_string(tracker.id));
		agent.add("type", tracker.type);
		agent.add("nickname", tracker.nickname);
		agent.add("baseURL", tracker.baseURL);
		agent.add("projectId", tracker.projectId);
		agent.add("trackerId", tracker.trackerId);
		agent.endObject();
	}
	agent.endObject();
}

template <>
void FaceRest::ResourceHandlerTemplate<RestStaticHandler>::handle(void)
{
	m_handler(this);
}

template <>
void FaceRest::ResourceHandlerTemplate<RestMemberHandler>::handle(void)
{
	(this->*m_handler)();
}

FaceRest::ResourceHandlerFactory::ResourceHandlerFactory(FaceRest *faceRest)
: m_faceRest(faceRest)
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
