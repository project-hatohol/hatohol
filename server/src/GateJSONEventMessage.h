/*
 * Copyright (C) 2014-2015 Project Hatohol
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
#include <json-glib/json-glib.h>

#include <string>
#include <list>
#include <memory>

#include <StringUtils.h>

#include <Params.h>
#include <Monitoring.h>

class GateJSONEventMessage {
public:
	GateJSONEventMessage(JsonNode *node);
	virtual ~GateJSONEventMessage();

	bool validate(mlpl::StringList &errors);

	int64_t getID();
	std::string getIDString();
	timespec getTimestamp();
	const char *getHostName();
	const char *getContent();
	TriggerSeverityType getSeverity();
	EventType getType();

	static const size_t EVENT_ID_DIGIT_NUM;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

