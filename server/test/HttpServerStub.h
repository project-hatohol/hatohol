/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#pragma once
#include <string>
#include <libsoup/soup.h>
#include "JSONParser.h"

class HttpServerStub {
public:
	HttpServerStub(const std::string &name = "HttpServerStub");
	virtual ~HttpServerStub();

	virtual bool isRunning(void);
	virtual void start(guint port);
	virtual void stop(void);
	virtual void reset(void);

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

