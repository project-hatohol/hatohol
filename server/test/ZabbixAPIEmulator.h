/*
 * Copyright (C) 2013 Project Hatohol
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

#ifndef ZabbixAPIEmulator_h
#define ZabbixAPIEmulator_h

#include <map>
#include <glib.h>
#include <libsoup/soup.h>
#include "JsonParserAgent.h"

enum OperationMode {
	OPE_MODE_NORMAL,
	OPE_MODE_HTTP_NOT_FOUND,
};

class ZabbixAPIEmulator {
public:
	struct APIHandlerArg;
	struct ParameterEventGet;
	struct JsonKeys;
	typedef void (ZabbixAPIEmulator::*APIHandler)(APIHandlerArg &);
	typedef map<int64_t, ZabbixAPIEmulator::JsonKeys> JsonData;
	typedef JsonData::iterator JsonDataIterator;

	ZabbixAPIEmulator(void);
	virtual ~ZabbixAPIEmulator();
	void reset(void);
	void setNumberOfEventSlices(size_t numSlices);

	bool isRunning(void);
	void start(guint port);
	void stop(void);
	void setOperationMode(OperationMode mode);

protected:
	static gpointer _mainThread(gpointer data);
	gpointer mainThread(void);
	static void startObject(JsonParserAgent &parser, const string &name);
	static void handlerDefault
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);
	static void handlerAPI
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);
	static bool hasParameter(APIHandlerArg &arg, const string &paramName,
	                         const string &expectedValue);

	string generateAuthToken(void);
	void handlerAPIDispatch(APIHandlerArg &arg);
	void APIHandlerGetWithFile(APIHandlerArg &arg, const string &dataFile);
	void APIHandlerUserLogin(APIHandlerArg &arg);
	void APIHandlerTriggerGet(APIHandlerArg &arg);
	void APIHandlerItemGet(APIHandlerArg &arg);
	void APIHandlerHostGet(APIHandlerArg &arg);
	void APIHandlerEventGet(APIHandlerArg &arg);
	void APIHandlerApplicationGet(APIHandlerArg &arg);
	void makeEventJsonData(const string &path);
	string addJsonResponse(const string &slice, APIHandlerArg &arg);
	void parseEventGetParameter(APIHandlerArg &arg);
	string setEventJsonData(JsonDataIterator jit);
private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ZabbixAPIEmulator_h
