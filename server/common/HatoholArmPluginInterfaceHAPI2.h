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

enum HAPI2ProcedureType {
	// Sv, Cl
	HAPI2_EXCHANGE_PROFILE,
	// Sv
	HAPI2_MONITORING_SERVER_INFO,
	HAPI2_LAST_INFO,
	HAPI2_PUT_ITEMS,
	HAPI2_PUT_HISTORY,
	HAPI2_UPDATE_HOSTS,
	HAPI2_UPDATE_HOST_GROUPS,
	HAPI2_UPDATE_HOST_GROUP_MEMEBRSHIP,
	HAPI2_UPDATE_TRIGGERS,
	HAPI2_UPDATE_EVENTS,
	HAPI2_UPDATE_HOST_PARENTS,
	HAPI2_UPDATE_ARM_INFO,
	// Cl
	HAPI2_FETCH_ITEMS,
	HAPI2_FETCH_HISTORY,
	HAPI2_FETCH_TRIGGERS,
	HAPI2_FETCH_EVENTS,

	// Client procedure type
	HAPI2_PROCEDURE_TYPE_HAP,
	// Invalid Type
	HAPI2_PROCEDURE_TYPE_BAD,
	// Sv -> Cl
	NUM_HAPI2_PROCEDURE
};

typedef std::vector<std::string>               ValidProcedureNameVect;
typedef ValidProcedureNameVect::iterator       ValidProcedureNameVectIterator;
typedef ValidProcedureNameVect::const_iterator ValidProcedureNameVectConstIterator;

class HatoholArmPluginInterfaceHAPI2 {

public:
	typedef enum {
		MODE_PLUGIN,
		MODE_SERVER
	} CommunicationMode;

	HatoholArmPluginInterfaceHAPI2(const CommunicationMode mode = MODE_PLUGIN);
	typedef std::string (HatoholArmPluginInterfaceHAPI2::*ProcedureHandler)
	  (const std::string &params);

	/**
	 * Register a procedure receive callback method.
	 * If the same code is specified more than twice, the handler is
	 * updated.
	 *
	 * @param type HAPI2ProcedureType
	 * @param handler A receive handler.
	 */
	void registerProcedureHandler(const HAPI2ProcedureType &type,
                                      ProcedureHandler handler);
	std::string interpretHandler(const HAPI2ProcedureType &type,
				     const std::string json);
	virtual void start(void);
	virtual void send(const std::string &procedure);

protected:
	typedef std::map<uint16_t, ProcedureHandler> ProcedureHandlerMap;
	typedef ProcedureHandlerMap::iterator	     ProcedureHandlerMapIterator;
	typedef ProcedureHandlerMap::const_iterator  ProcedureHandlerMapConstIterator;

	void setArmPluginInfo(const ArmPluginInfo &pluginInfo);

	virtual void onHandledCommand(const HAPI2ProcedureType &type);

	std::mt19937 getRandomEngine(void);

	static bool setResponseId(JSONParser &requestParser,
				  JSONBuilder &responseBuilder);
	std::string buildErrorResponse(const int errorCode,
				       const std::string errorMessage,
				       JSONParser *requestParser = NULL);

	virtual ~HatoholArmPluginInterfaceHAPI2();

private:
	class AMQPHAPI2MessageHandler;
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // HatoholArmPluginInterfaceHAPI2_h
