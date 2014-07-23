/*
 * Copyright (C) 2014 Project Hatohol
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

#include "HatoholDBUtils.h"
#include "DBClientHatohol.h"

enum {
	EVENT_OBJECT_TRIGGER = 0,
	EVENT_OBJECT_DHOST,
	EVENT_OBJECT_DSERVICE,
	EVENT_OBJECT_ZABBIX_ACTIVE
};


void HatoholDBUtils::transformEventsToHatoholFormat(
  EventInfoList &eventInfoList, const ItemTablePtr events,
  const ServerIdType &serverId)
{
	const ItemGroupList &itemGroupList = events->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		EventInfo eventInfo;
		initEventInfo(eventInfo);
		eventInfo.serverId = serverId;
		if (!transformEventItemGroupToEventInfo(eventInfo, *it))
			continue;
		eventInfoList.push_back(eventInfo);
	}
}

bool HatoholDBUtils::transformEventItemGroupToEventInfo
  (EventInfo &eventInfo, const ItemGroup *eventItemGroup)
{
	ItemGroupStream itemGroupStream(eventItemGroup);

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_EVENTID);
	itemGroupStream >> eventInfo.id;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_OBJECT);
	if (itemGroupStream.read<int>() != EVENT_OBJECT_TRIGGER)
		return false;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_OBJECTID);
	itemGroupStream >> eventInfo.triggerId;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_CLOCK);
	itemGroupStream >> eventInfo.time.tv_sec;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_VALUE);
	itemGroupStream >> eventInfo.type;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_NS);
	itemGroupStream >> eventInfo.time.tv_nsec;

	// Trigger's value. This can be transformed from Event's value
	// This value is refered in ActionManager. So we set here.
	switch (eventInfo.type) {
	case EVENT_TYPE_GOOD:
		eventInfo.status = TRIGGER_STATUS_OK;
		break;
	case EVENT_TYPE_BAD:
		eventInfo.status = TRIGGER_STATUS_PROBLEM;
		break;
	case EVENT_TYPE_UNKNOWN:
		eventInfo.status = TRIGGER_STATUS_UNKNOWN;
		break;
	default:
		MLPL_ERR("Unknown type: %d\n", eventInfo.type);
		eventInfo.status = TRIGGER_STATUS_UNKNOWN;
	}

	return true;
}
