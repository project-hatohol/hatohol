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

#ifndef HttpServer_h
#define HttpServer_h

#include <string>
#include <libsoup/soup.h>
#include "JsonParserAgent.h"

class HttpServerStub {
public:
	HttpServerStub(const std::string &name = "HttpServerStub");
	virtual ~HttpServerStub();

	virtual bool isRunning(void);
	virtual void start(guint port);
	virtual void stop(void);

protected:
	virtual gpointer mainThread(void);
	virtual void setSoupHandlers(SoupServer *server);

	static gpointer _mainThread(gpointer data);
	static void handlerDefault
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // HttpServer_h
