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

#pragma once
#include <libsoup/soup.h>
#include "FaceBase.h"
#include "JSONBuilder.h"
#include "JSONParser.h"
#include "SmartTime.h"
#include "Params.h"
#include "HatoholError.h"
#include "DBTablesConfig.h"
#include "DBTablesUser.h"
#include "DBTablesMonitoring.h"
#include "Closure.h"
#include "Utils.h"

struct FaceRestParam {
	virtual void setupDoneNotifyFunc(void)
	{
	}
};

class FaceRest : public FaceBase {
public:
	struct ResourceHandlerFactory;
	template<typename T> struct ResourceHandlerFactoryTemplate;
	struct ResourceHandler;
	template<typename T> struct ResourceHandlerTemplate;

	static int API_VERSION;
	static const char *SESSION_ID_HEADER_NAME;
	static const int DEFAULT_NUM_WORKERS;

	static void init(void);

	FaceRest(FaceRestParam *param = NULL);
	virtual ~FaceRest();
	virtual void waitExit(void) override;
	virtual void setNumberOfPreLoadWorkers(size_t num);

	void addResourceHandlerFactory(const char *path,
				       ResourceHandlerFactory *factory);

protected:
	class Worker;

	// virtual methods
	gpointer mainThread(HatoholThreadArg *arg) override;

	// for async mode
	bool isAsyncMode(void);
	void startWorkers(void);
	void stopWorkers(void);

	// generic sub routines
	SoupServer   *getSoupServer(void);
	GMainContext *getGMainContext(void);

	// handlers
	static void
	  handlerDefault(SoupServer *server, SoupMessage *msg,
	                 const char *path, GHashTable *query,
	                 SoupClientContext *client, gpointer user_data);
	static void handlerHelloPage(ResourceHandler *job);
	static void handlerTest(ResourceHandler *job);
	static void handlerLogin(ResourceHandler *job);
	static void handlerLogout(ResourceHandler *job);

	int onCaughtException(const std::exception &e) override
	{
		return EXIT_FATAL;
	}

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	static const char *pathForTest;
	static const char *pathForLogin;
	static const char *pathForLogout;
};

