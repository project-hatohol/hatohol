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

#ifndef FaceRest_h
#define FaceRest_h

#include <libsoup/soup.h>
#include "FaceBase.h"
#include "JsonBuilderAgent.h"
#include "SmartTime.h"
#include "Params.h"
#include "HatoholError.h"
#include "DBClientConfig.h"
#include "DBClientUser.h"
#include "DBClientHatohol.h"
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
	struct ResourceHandler;

	static int API_VERSION;
	static const char *SESSION_ID_HEADER_NAME;
	static const int DEFAULT_NUM_WORKERS;

	static void init(void);
	static void reset(const CommandLineArg &arg);
	static bool isTestMode(void);

	FaceRest(CommandLineArg &cmdArg, FaceRestParam *param = NULL);
	virtual ~FaceRest();
	virtual void waitExit(void) override;
	virtual void setNumberOfPreLoadWorkers(size_t num);

	void addResourceHandlerFactory(const char *path,
				       ResourceHandlerFactory *factory);

protected:
	class Worker;

	// virtual methods
	gpointer mainThread(HatoholThreadArg *arg);

	// for async mode
	bool isAsyncMode(void);
	void startWorkers(void);
	void stopWorkers(void);

	// generic sub routines
	SoupServer   *getSoupServer(void);
	GMainContext *getGMainContext(void);
	size_t parseCmdArgPort(CommandLineArg &cmdArg, size_t idx);

	// handlers
	static void
	  handlerDefault(SoupServer *server, SoupMessage *msg,
	                 const char *path, GHashTable *query,
	                 SoupClientContext *client, gpointer user_data);
	static void handlerHelloPage(ResourceHandler *job);
	static void handlerTest(ResourceHandler *job);
	static void handlerLogin(ResourceHandler *job);
	static void handlerLogout(ResourceHandler *job);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

	static const char *pathForTest;
	static const char *pathForLogin;
	static const char *pathForLogout;
};

#endif // FaceRest_h
