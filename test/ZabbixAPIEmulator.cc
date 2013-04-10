#include <cstdio>
#include <Logger.h>

#include "ZabbixAPIEmulator.h"

using namespace mlpl;

struct ZabbixAPIEmulator::PrivateContext {
	GThread    *thread;
	guint       port;
	SoupServer *soupServer;
	
	// methods
	PrivateContext(void)
	: thread(NULL),
	  port(0),
	  soupServer(NULL)
	{
	}

	virtual ~PrivateContext()
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ZabbixAPIEmulator::ZabbixAPIEmulator(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ZabbixAPIEmulator::~ZabbixAPIEmulator()
{
	if (isRunning()) {
		soup_server_disconnect(m_ctx->soupServer);
		g_object_unref(m_ctx->soupServer);
		m_ctx->soupServer = NULL;
	}
	if (m_ctx)
		delete m_ctx;
}

bool ZabbixAPIEmulator::isRunning(void)
{
	return m_ctx->thread;
}

void ZabbixAPIEmulator::start(guint port)
{
	if (isRunning()) {
		MLPL_WARN("Thread is already running.");
		return;
	}
	
	m_ctx->soupServer = soup_server_new(SOUP_SERVER_PORT, port, NULL);
	soup_server_add_handler(m_ctx->soupServer, NULL, handlerDefault,
	                        this, NULL);
	m_ctx->thread = g_thread_new("ZabbixAPIEmulator", _mainThread, this);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer ZabbixAPIEmulator::_mainThread(gpointer data)
{
	ZabbixAPIEmulator *obj = static_cast<ZabbixAPIEmulator *>(data);
	return obj->mainThread();
}

gpointer ZabbixAPIEmulator::mainThread(void)
{
	soup_server_run(m_ctx->soupServer);
	return NULL;
}

void ZabbixAPIEmulator::handlerDefault
  (SoupServer *server, SoupMessage *msg, const char *path, GHashTable *query,
   SoupClientContext *client, gpointer user_data)
{
	MLPL_DBG("Default handler: path: %s, method: %s\n",
	         path, msg->method);
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}
