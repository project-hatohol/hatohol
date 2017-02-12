/*
 * Copyright (C) 2015 Project Hatohol
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
#include <random>
#include "HatoholThreadBase.h"
#include "HatoholException.h"
#include "JSONParser.h"
#include "JSONBuilder.h"
#include "ItemDataPtr.h"
#include "ItemGroupPtr.h"
#include "ItemTablePtr.h"
#include "Utils.h"
#include "MonitoringServerInfo.h"
#include "ArmPluginInfo.h"

// Invalid JSON was received by the server.
// An error occurred on the server while parsing the JSON text.
const int JSON_RPC_PARSE_ERROR = -32700;

// The JSON sent is not a valid Request object.
const int JSON_RPC_INVALID_REQUEST  = -32600;

// The method does not exist / is not available.
const int JSON_RPC_METHOD_NOT_FOUND = -32601;

//  Invalid method parameter(s).
const int JSON_RPC_INVALID_PARAMS = -32602;

// Internal JSON-RPC error.
const int JSON_RPC_INTERNAL_ERROR = -32603;

// Reserved for implementation-defined server-errors.
const int JSON_RPC_SERVER_ERROR_BEGIN = -32000;
const int JSON_RPC_SERVER_ERROR_END = -32099;

typedef std::string HAPI2ProcedureName;

// Sv, Cl
const HAPI2ProcedureName HAPI2_EXCHANGE_PROFILE
  = "exchangeProfile";
// Sv
const HAPI2ProcedureName HAPI2_MONITORING_SERVER_INFO
  = "getMonitoringServerInfo";
const HAPI2ProcedureName HAPI2_LAST_INFO
  = "getLastInfo";
const HAPI2ProcedureName HAPI2_PUT_ITEMS
  = "putItems";
const HAPI2ProcedureName HAPI2_PUT_HISTORY
  = "putHistory";
const HAPI2ProcedureName HAPI2_PUT_HOSTS
  = "putHosts";
const HAPI2ProcedureName HAPI2_PUT_HOST_GROUPS
  = "putHostGroups";
const HAPI2ProcedureName HAPI2_PUT_HOST_GROUP_MEMEBRSHIP
  = "putHostGroupMembership";
const HAPI2ProcedureName HAPI2_PUT_TRIGGERS
  = "putTriggers";
const HAPI2ProcedureName HAPI2_PUT_EVENTS
  = "putEvents";
const HAPI2ProcedureName HAPI2_PUT_HOST_PARENTS
  = "putHostParents";
const HAPI2ProcedureName HAPI2_PUT_ARM_INFO
  = "putArmInfo";
// Cl
const HAPI2ProcedureName HAPI2_FETCH_ITEMS
  = "fetchItems";
const HAPI2ProcedureName HAPI2_FETCH_HISTORY
  = "fetchHistory";
const HAPI2ProcedureName HAPI2_FETCH_TRIGGERS
  = "fetchTriggers";
const HAPI2ProcedureName HAPI2_FETCH_EVENTS
  = "fetchEvents";
const HAPI2ProcedureName HAPI2_UPDATE_MONITORING_SERVER_INFO
  = "updateMonitoringServerInfo";

enum MethodOwner {
	SERVER,
	HAP,
	BOTH
};

enum MethodType {
	PROCEDURE,
	NOTIFICATION
};

enum MethodRequirementLevel {
	MANDATORY,
	OPTIONAL
};

struct HAPI2ProcedureDef {
	MethodOwner owner;
	MethodType type;
	std::string name;
	MethodRequirementLevel level;
};

enum class HAPI2PluginCollectType {
	NG_PARSER_ERROR = 0,
	NG_DISCONNECT,
	NG_PLUGIN_INTERNAL_ERROR,
	NG_HATOHOL_INTERNAL_ERROR,
	NG_PLUGIN_CONNECT_ERROR,
	NG_AMQP_CONNECT_ERROR,
	NUM_COLLECT_NG_KIND,
	OK,
};

enum class HAPI2PluginErrorCode {
	OK,
	HAP2_CONNECTION_UNAVAILABLE,
	UNAVAILABLE_HAP2,
	UNKNOWN,
};

class HatoholArmPluginInterfaceHAPI2 {

public:
	typedef enum {
		MODE_PLUGIN,
		MODE_SERVER
	} CommunicationMode;

	HatoholArmPluginInterfaceHAPI2(const CommunicationMode mode = MODE_PLUGIN);
	typedef std::string (HatoholArmPluginInterfaceHAPI2::*ProcedureHandler)
	  (JSONParser &parser);

	class ProcedureCallback {
	public:
		virtual void onGotResponse(JSONParser &parser) = 0;
		virtual void onTimeout(void) {};
	};

	/**
	 * Register a procedure receive callback method.
	 * If the same code is specified more than twice, the handler is
	 * updated.
	 *
	 * @param type HAPI2ProcedureName
	 * @param handler A receive handler.
	 */
	void registerProcedureHandler(const HAPI2ProcedureName &type,
				      ProcedureHandler handler);
	std::string interpretHandler(const HAPI2ProcedureName &type,
				     JSONParser &parser);
	void handleResponse(const std::string id, JSONParser &parser);

	virtual void start(void);
	virtual void stop(void);
	virtual void send(const std::string &message);
	virtual void send(const std::string &message,
			  const int64_t id,
			  std::shared_ptr<ProcedureCallback> callback);

	virtual bool getEstablished(void);
	virtual void setEstablished(bool established);

	const std::list<HAPI2ProcedureDef> &getProcedureDefList(void) const;

protected:
	typedef std::map<HAPI2ProcedureName, ProcedureHandler> ProcedureHandlerMap;

	virtual ~HatoholArmPluginInterfaceHAPI2();

	void setArmPluginInfo(const ArmPluginInfo &pluginInfo);
	std::mt19937 getRandomEngine(void);
	static bool setResponseId(JSONParser &requestParser,
				  JSONBuilder &responseBuilder);
	std::string buildErrorResponse(
	  const int errorCode,
	  const std::string errorMessage,
	  const mlpl::StringList *detailedMessages = NULL,
	  JSONParser *requestParser = NULL);
	virtual void onSetPluginInitialInfo(void);
	virtual void onConnect(void);
	virtual void onConnectFailure(void);

private:
	class AMQPHAPI2MessageHandler;
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

