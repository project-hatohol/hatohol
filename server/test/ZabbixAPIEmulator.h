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
#include <map>
#include <glib.h>
#include <libsoup/soup.h>
#include "JSONParser.h"
#include "HttpServerStub.h"
#include "HostInfoCache.h"

enum OperationMode {
	OPE_MODE_NORMAL,
	OPE_MODE_HTTP_NOT_FOUND,
};

class ZabbixAPIEmulator : public HttpServerStub {
public:
	enum APIVersion {
		API_VERSION_1_3_0, // Zabbix 1.8
		API_VERSION_1_4_0, // Zabbix 2.0.0 - 2.0.3
		API_VERSION_2_0_4, // default
		API_VERSION_2_2_0,
		API_VERSION_2_2_2,
		API_VERSION_2_3_0, // Should be renamed to 2_4_0 after
		                   // Zabbix 2.4.0 is released
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

	void setOperationMode(OperationMode mode);
	void setAPIVersion(APIVersion version);
	std::string getAPIVersionString(void);
	void setExpectedFirstEventId(const uint64_t &id);
	void setExpectedLastEventId(const uint64_t &id);

	static std::string getAPIVersionString(APIVersion version);
	static void loadHostInfoCache(HostInfoCache &hostInfoCache,
	                              const ServerIdType &serverId);

protected:
	virtual void setSoupHandlers(SoupServer *soupServer);
	static void startObject(JSONParser &parser,
	                        const std::string &name);
	static void handlerAPI
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);
	static bool hasParameter(APIHandlerArg &arg,
	                         const std::string &paramName,
	                         const std::string &expectedValue);
	static bool hasParameter(APIHandlerArg &arg,
	                         const std::string &paramName,
	                         const int64_t &expectedValue);

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
	void APIHandlerHistoryGet(APIHandlerArg &arg);
	void makeEventJSONData(const std::string &path);
	std::string addJSONResponse(const std::string &slice,
	                            APIHandlerArg &arg);
	void parseEventGetParameter(APIHandlerArg &arg);
	void loadTestEventsIfNeeded(APIHandlerArg &arg);
private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

