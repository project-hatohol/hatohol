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
	enum APIVersion {
		API_VERSION_1_3_0, // Zabbix 1.8
		API_VERSION_1_4_0, // Zabbix 2.0.0 - 2.0.3
		API_VERSION_2_0_4, // default
		API_VERSION_2_2_0,
	};

	struct APIHandlerArg;
	struct ParameterEventGet;
	struct ZabbixAPIEvent;
	typedef void (ZabbixAPIEmulator::*APIHandler)(APIHandlerArg &);

	// key: Event ID
	typedef std::map<int64_t, ZabbixAPIEvent>   ZabbixAPIEventMap;
	typedef ZabbixAPIEventMap::iterator         ZabbixAPIEventMapIterator;
	typedef ZabbixAPIEventMap::reverse_iterator ZabbixAPIEventMapReverseIterator;

	ZabbixAPIEmulator(void);
	virtual ~ZabbixAPIEmulator();
	void reset(void);
	void setNumberOfEventSlices(size_t numSlices);

	bool isRunning(void);
	void start(guint port);
	void stop(void);
	void setOperationMode(OperationMode mode);
	void setAPIVersion(APIVersion version);
	std::string getAPIVersionString(void);

	static std::string getAPIVersionString(APIVersion version);

protected:
	static gpointer _mainThread(gpointer data);
	gpointer mainThread(void);
	static void startObject(JsonParserAgent &parser,
	                        const std::string &name);
	static void handlerDefault
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);
	static void handlerAPI
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);
	static bool hasParameter(APIHandlerArg &arg,
	                         const std::string &paramName,
	                         const std::string &expectedValue);

	std::string generateAuthToken(void);
	void handlerAPIDispatch(APIHandlerArg &arg);
	void APIHandlerGetWithFile(APIHandlerArg &arg,
	                           const std::string &dataFile);
	void APIHandlerAPIVersion(APIHandlerArg &arg);
	void APIHandlerUserLogin(APIHandlerArg &arg);
	void APIHandlerTriggerGet(APIHandlerArg &arg);
	void APIHandlerItemGet(APIHandlerArg &arg);
	void APIHandlerHostGet(APIHandlerArg &arg);
	void APIHandlerEventGet(APIHandlerArg &arg);
	void APIHandlerApplicationGet(APIHandlerArg &arg);
	void APIHandlerHostgroupGet(APIHandlerArg &arg);
	void makeEventJsonData(const std::string &path);
	std::string addJsonResponse(const std::string &slice,
	                            APIHandlerArg &arg);
	void parseEventGetParameter(APIHandlerArg &arg);
	std::string makeJsonString(const ZabbixAPIEvent &data);
	void loadTestEventsIfNeeded(APIHandlerArg &arg);
private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ZabbixAPIEmulator_h
