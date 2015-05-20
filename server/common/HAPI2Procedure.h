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

#ifndef HAPI2Procedure_h
#define HAPI2Procedure_h

#include <json-glib/json-glib.h>

#include <string>
#include <list>
#include <memory>

#include <StringUtils.h>

#include <Params.h>
#include <Monitoring.h>
#include "HatoholArmPluginInterfaceHAPI2.h"

class HAPI2Procedure {
public:
	HAPI2Procedure(JsonNode *node);
	virtual ~HAPI2Procedure();

	bool validate(mlpl::StringList &errors);

	HAPI2ProcedureType getType();
	std::string getParams();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};


#endif // HAPI2Procedure_h
