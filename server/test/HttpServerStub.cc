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

#include "HttpServerStub.h"

struct HttpServerStub::PrivateContext {
	std::string name;
	GThread    *thread;
	guint       port;
	SoupServer *soupServer;
	GMainContext   *gMainCtx;
	
	// methods
	PrivateContext(void)
	: thread(NULL),
	  port(0),
	  soupServer(NULL),
	  gMainCtx(0)
	{
		gMainCtx = g_main_context_new();
	}

	virtual ~PrivateContext()
	{
		if (thread) {
#ifdef GLIB_VERSION_2_32
			g_thread_unref(thread);
#else
			// nothing to do
#endif // GLIB_VERSION_2_32
		}
		if (gMainCtx)
			g_main_context_unref(gMainCtx);
	}
};

HttpServerStub::HttpServerStub(const std::string &name)
{
	m_ctx = new PrivateContext();
	m_ctx->name = name;
}

HttpServerStub::~HttpServerStub()
{
	if (isRunning()) {
		soup_server_quit(m_ctx->soupServer);
		g_object_unref(m_ctx->soupServer);
		m_ctx->soupServer = NULL;
	}
	if (m_ctx)
		delete m_ctx;
}

bool HttpServerStub::isRunning(void)
{
	return m_ctx->thread;
}

void HttpServerStub::start(guint port)
{
	if (isRunning()) {
		MLPL_WARN("Thread is already running.");
		return;
	}
	
	m_ctx->soupServer =
	  soup_server_new(SOUP_SERVER_PORT, port,
	                  SOUP_SERVER_ASYNC_CONTEXT, m_ctx->gMainCtx, NULL);
	soup_server_add_handler(m_ctx->soupServer, NULL, handlerDefault,
	                        this, NULL);
	setSoupHandlers(m_ctx->soupServer);

#ifdef GLIB_VERSION_2_32
	m_ctx->thread = g_thread_new(m_ctx->name.c_str(), _mainThread, this);
#else
	m_ctx->thread = g_thread_create(_mainThread, this, TRUE, NULL);
#endif // GLIB_VERSION_2_32
}

void HttpServerStub::stop(void)
{
	soup_server_quit(m_ctx->soupServer);
	g_thread_join(m_ctx->thread);
	m_ctx->thread = NULL;
	g_object_unref(m_ctx->soupServer);
	m_ctx->soupServer = NULL;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer HttpServerStub::_mainThread(gpointer data)
{
	HttpServerStub *obj = static_cast<HttpServerStub *>(data);
	return obj->mainThread();
}

gpointer HttpServerStub::mainThread(void)
{
	soup_server_run(m_ctx->soupServer);
	return NULL;
}

void HttpServerStub::setSoupHandlers(SoupServer *server)
{
}

void HttpServerStub::handlerDefault
  (SoupServer *server, SoupMessage *msg, const char *path, GHashTable *query,
   SoupClientContext *client, gpointer user_data)
{
	MLPL_DBG("Default handler: path: %s, method: %s\n",
	         path, msg->method);
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}
