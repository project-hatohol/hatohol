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
#include "HatoholArmPluginInterfaceHAPI2.h"

class GateJSONProcedureHAPI2 {
public:
	GateJSONProcedureHAPI2(JsonNode *node);
	virtual ~GateJSONProcedureHAPI2();

	bool validate(mlpl::StringList &errors);

	HAPI2ProcedureType getProcedureType();
	std::string getParams();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};


#endif // GateJSONProcedureHAPI2_h