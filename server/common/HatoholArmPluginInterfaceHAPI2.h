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

#include <json-glib/json-glib.h>

#include <string>
#include "HatoholThreadBase.h"
#include "HatoholException.h"
#include "ItemDataPtr.h"
#include "ItemGroupPtr.h"
#include "ItemTablePtr.h"
#include "Utils.h"
#include "MonitoringServerInfo.h"

enum HAPI2ProcedureType {
	HAPI2_EXCHANGE_PROFILE,
	HAPI2_MONITORING_SERVER_INFO,
	HAPI2_LAST_INFO,
	HAPI2_PUT_ITEMS,
	HAPI2_PUT_HISTORY,
	HAPI2_UPDATE_HOSTS,
	HAPI2_UPDATE_HOST_GROUPS,
	HAPI2_UPDATE_HOST_GROUP_MEMEBRSHIP,
	HAPI2_UPDATE_TRIGGERS,
	HAPI2_UPDATE_EVENTS,
	HAPI2_UPDATE_HOST_PARENT,
	HAPI2_UPDATE_ARM_INFO,
	HAPI2_FETCH_ITEMS,
	HAPI2_FETCH_HISTORY,
	HAPI2_FETCH_TRIGGERS,
	HAPI2_FETCH_EVENTS,

	// Invalid Type
	HAPI2_PROCEDURE_TYPE_BAD,
	// Sv -> Cl
	NUM_HAPI2_PROCEDURE
};

class HatoholArmPluginInterfaceHAPI2 {

public:
	HatoholArmPluginInterfaceHAPI2();
	typedef void (HatoholArmPluginInterfaceHAPI2::*ProcedureHandler)
	  (const HAPI2ProcedureType *type);
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
	void interpretHandler(const HAPI2ProcedureType &type, JsonObject &message);

protected:
	typedef std::map<uint16_t, ProcedureHandler> ProcedureHandlerMap;
	typedef ProcedureHandlerMap::iterator	     ProcedureHandlerMapIterator;
	typedef ProcedureHandlerMap::const_iterator  ProcedureHandlerMapConstIterator;

	virtual void onHandledCommand(const HAPI2ProcedureType &type);

        virtual ~HatoholArmPluginInterfaceHAPI2();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // HatoholArmPluginInterfaceHAPI2_h
