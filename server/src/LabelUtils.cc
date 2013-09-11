/*
 * Copyright (C) 2013 Project Hatohol
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

#include "LabelUtils.h"
#include "StringUtils.h"
using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
string LabelUtils::getEventTypeLabel(EventType eventType)
{
	switch (eventType) {
	case EVENT_TYPE_ACTIVATED:
		return "Activated";
	case EVENT_TYPE_DEACTIVATED:
		return "Deactivated";
	case EVENT_TYPE_UNKNOWN:
		return "Unknown";
	default:
		return StringUtils::sprintf("Invalid: %d", eventType);
	}
}

string LabelUtils::getTriggerStatusLabel(TriggerStatusType status)
{
	switch (status) {
	case TRIGGER_STATUS_OK:
		return "OK";
	case TRIGGER_STATUS_PROBLEM:
		return "PROBLEM";
	default:
		return StringUtils::sprintf("Invalid: %d", status);
	}
}

string LabelUtils::getTriggerSeverityLabel(TriggerSeverityType severity)
{
	switch (severity) {
	case TRIGGER_SEVERITY_INFO:
		return "INFO";
	case TRIGGER_SEVERITY_WARN:
		return "WARN";
	case TRIGGER_SEVERITY_CRITICAL:
		return "CRITICAL";
	case TRIGGER_SEVERITY_UNKNOWN:
		return "UNKNWON";
	default:
		return StringUtils::sprintf("Invalid: %d", severity);
	}
}
