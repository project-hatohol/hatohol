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

#ifndef GateJSONProcedureHAPI2_h
#define GateJSONProcedureHAPI2_h

#include <json-glib/json-glib.h>

#include <string>
#include <list>
#include <memory>

#include <StringUtils.h>

#include <Params.h>
#include <Monitoring.h>

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

class GateJSONProcedureHAPI2 {
public:
	GateJSONProcedureHAPI2(JsonNode *node);
	virtual ~GateJSONProcedureHAPI2();

	bool validate(mlpl::StringList &errors);

	HAPI2ProcedureType getProcedureType();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // GateJSONProcedureHAPI2_h
