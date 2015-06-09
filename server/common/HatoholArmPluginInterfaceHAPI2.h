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

#ifndef HatoholArmPluginInterfaceHAPI2_h
#define HatoholArmPluginInterfaceHAPI2_h

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
const HAPI2ProcedureName HAPI2_UPDATE_HOSTS
  = "updateHosts";
const HAPI2ProcedureName HAPI2_UPDATE_HOST_GROUPS
  = "updateHostGroups";
const HAPI2ProcedureName HAPI2_UPDATE_HOST_GROUP_MEMEBRSHIP
  = "updateHostGroupMembership";
const HAPI2ProcedureName HAPI2_UPDATE_TRIGGERS
  = "updateTriggers";
const HAPI2ProcedureName HAPI2_UPDATE_EVENTS
  = "updateEvents";
const HAPI2ProcedureName HAPI2_UPDATE_HOST_PARENTS
  = "updateHostParent";
const HAPI2ProcedureName HAPI2_UPDATE_ARM_INFO
  = "updateArmInfo";
// Cl
const HAPI2ProcedureName HAPI2_FETCH_ITEMS
  = "fetchItems";
const HAPI2ProcedureName HAPI2_FETCH_HISTORY
  = "fetchHistory";
const HAPI2ProcedureName HAPI2_FETCH_TRIGGERS
  = "fetchTriggers";
const HAPI2ProcedureName HAPI2_FETCH_EVENTS
  = "fetchEvents";

enum ProcedureImplementType {
	PROCEDURE_SERVER,
	PROCEDURE_HAP,
	PROCEDURE_BOTH
};

enum ProcedureRequirementLevel {
	BOTH_MANDATORY,
	SERVER_MANDATORY,
	SERVER_OPTIONAL,
	SERVER_MANDATORY_HAP_OPTIONAL,
	HAP_MANDATORY,
	HAP_OPTIONAL
};

struct HAPI2ProcedureDef {
	ProcedureImplementType type;
	std::string name;
	ProcedureRequirementLevel level;
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

	class ProcedureCallback : public UsedCountable {
	public:
		virtual void onGotResponse(JSONParser &parser) = 0;
	};
	typedef UsedCountablePtr<ProcedureCallback> ProcedureCallbackPtr;

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
	virtual void send(const std::string &message);
	virtual void send(const std::string &message,
			  const int64_t id,
			  ProcedureCallbackPtr callback);

	const std::list<HAPI2ProcedureDef> &getDefaultValidProcedureList(void) const;

protected:
	typedef std::map<HAPI2ProcedureName, ProcedureHandler> ProcedureHandlerMap;

	virtual ~HatoholArmPluginInterfaceHAPI2();

	void setArmPluginInfo(const ArmPluginInfo &pluginInfo);
	std::mt19937 getRandomEngine(void);
	static bool setResponseId(JSONParser &requestParser,
				  JSONBuilder &responseBuilder);
	std::string buildErrorResponse(const int errorCode,
				       const std::string errorMessage,
				       JSONParser *requestParser = NULL);

private:
	class AMQPHAPI2MessageHandler;
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // HatoholArmPluginInterfaceHAPI2_h
