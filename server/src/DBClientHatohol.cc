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

#include <memory>

#include <MutexLock.h>
using namespace mlpl;

#include "DBAgentFactory.h"
#include "DBClientHatohol.h"
#include "ConfigManager.h"
#include "DBClientUtils.h"

static const char *TABLE_NAME_TRIGGERS = "triggers";
static const char *TABLE_NAME_EVENTS   = "events";
static const char *TABLE_NAME_ITEMS    = "items";

uint64_t DBClientHatohol::EVENT_NOT_FOUND = -1;
int DBClientHatohol::HATOHOL_DB_VERSION = 3;

static const ColumnDef COLUMN_DEF_TRIGGERS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"last_change_time_sec",            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"last_change_time_ns",             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"hostname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"brief",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

static const size_t NUM_COLUMNS_TRIGGERS =
  sizeof(COLUMN_DEF_TRIGGERS) / sizeof(ColumnDef);

enum {
	IDX_TRIGGERS_SERVER_ID,
	IDX_TRIGGERS_ID,
	IDX_TRIGGERS_STATUS,
	IDX_TRIGGERS_SEVERITY,
	IDX_TRIGGERS_LAST_CHANGE_TIME_SEC,
	IDX_TRIGGERS_LAST_CHANGE_TIME_NS,
	IDX_TRIGGERS_HOST_ID,
	IDX_TRIGGERS_HOSTNAME,
	IDX_TRIGGERS_BRIEF,
	NUM_IDX_TRIGGERS,
};

static const ColumnDef COLUMN_DEF_EVENTS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"time_sec",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"time_ns",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"event_value",                     // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"hostname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"brief",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

static const size_t NUM_COLUMNS_EVENTS =
  sizeof(COLUMN_DEF_EVENTS) / sizeof(ColumnDef);

enum {
	IDX_EVENTS_SERVER_ID,
	IDX_EVENTS_ID,
	IDX_EVENTS_TIME_SEC,
	IDX_EVENTS_TIME_NS,
	IDX_EVENTS_EVENT_TYPE,
	IDX_EVENTS_TRIGGER_ID,
	IDX_EVENTS_STATUS,
	IDX_EVENTS_SEVERITY,
	IDX_EVENTS_HOST_ID,
	IDX_EVENTS_HOST_NAME,
	IDX_EVENTS_BRIEF,
	NUM_IDX_EVENTS,
};

static const ColumnDef COLUMN_DEF_ITEMS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"brief",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"last_value_time_sec",             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"last_value_time_ns",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"last_value",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"prev_value",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"item_group_name",                 // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

static const size_t NUM_COLUMNS_ITEMS =
  sizeof(COLUMN_DEF_ITEMS) / sizeof(ColumnDef);

enum {
	IDX_ITEMS_SERVER_ID,
	IDX_ITEMS_ID,
	IDX_ITEMS_HOST_ID,
	IDX_ITEMS_BRIEF,
	IDX_ITEMS_LAST_VALUE_TIME_SEC,
	IDX_ITEMS_LAST_VALUE_TIME_NS,
	IDX_ITEMS_LAST_VALUE,
	IDX_ITEMS_PREV_VALUE,
	IDX_ITEMS_ITEM_GROUP_NAME,
	NUM_IDX_ITEMS,
};

struct DBClientHatohol::PrivateContext
{
	static MutexLock mutex;
	static bool   initialized;

	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}

	static void lock(void)
	{
		mutex.lock();
	}

	static void unlock(void)
	{
		mutex.unlock();
	}
};
MutexLock DBClientHatohol::PrivateContext::mutex;
bool   DBClientHatohol::PrivateContext::initialized = false;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientHatohol::init(void)
{
	HATOHOL_ASSERT(NUM_COLUMNS_TRIGGERS == NUM_IDX_TRIGGERS,
	  "NUM_COLUMNS_TRIGGERS: %zd, NUM_IDX_TRIGGERS: %d",
	  NUM_COLUMNS_TRIGGERS, NUM_IDX_TRIGGERS);

	HATOHOL_ASSERT(NUM_COLUMNS_EVENTS == NUM_IDX_EVENTS,
	  "NUM_COLUMNS_EVENTS: %zd, NUM_IDX_EVENTS: %d",
	  NUM_COLUMNS_EVENTS, NUM_IDX_EVENTS);

	HATOHOL_ASSERT(NUM_COLUMNS_ITEMS == NUM_IDX_ITEMS,
	  "NUM_COLUMNS_ITEMS: %zd, NUM_IDX_ITEMS: %d",
	  NUM_COLUMNS_ITEMS, NUM_IDX_ITEMS);
}

void DBClientHatohol::reset(void)
{
	resetDBInitializedFlags();
}

DBClientHatohol::DBClientHatohol(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();

	m_ctx->lock();
	if (!m_ctx->initialized) {
		// The setup function: dbSetupFunc() is called from
		// the creation of DBAgent instance below.
		prepareSetupFunction();
	}
	m_ctx->unlock();
	setDBAgent(DBAgentFactory::create(DB_DOMAIN_ID_HATOHOL));
}

DBClientHatohol::~DBClientHatohol()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientHatohol::addTriggerInfo(TriggerInfo *triggerInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addTriggerInfoBare(*triggerInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addTriggerInfoList(const TriggerInfoList &triggerInfoList)
{
	TriggerInfoListConstIterator it = triggerInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != triggerInfoList.end(); ++it)
			addTriggerInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::getTriggerInfoList(TriggerInfoList &triggerInfoList)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;
	arg.columnDefs = COLUMN_DEF_TRIGGERS;
	arg.columnIndexes.push_back(IDX_TRIGGERS_SERVER_ID);
	arg.columnIndexes.push_back(IDX_TRIGGERS_ID);
	arg.columnIndexes.push_back(IDX_TRIGGERS_STATUS);
	arg.columnIndexes.push_back(IDX_TRIGGERS_SEVERITY);
	arg.columnIndexes.push_back(IDX_TRIGGERS_LAST_CHANGE_TIME_SEC);
	arg.columnIndexes.push_back(IDX_TRIGGERS_LAST_CHANGE_TIME_NS);
	arg.columnIndexes.push_back(IDX_TRIGGERS_HOST_ID);
	arg.columnIndexes.push_back(IDX_TRIGGERS_HOSTNAME);
	arg.columnIndexes.push_back(IDX_TRIGGERS_BRIEF);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		triggerInfoList.push_back(TriggerInfo());
		TriggerInfo &trigInfo = triggerInfoList.back();

		trigInfo.serverId  = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.id        = GET_UINT64_FROM_GRP(itemGroup, idx++);
		int status         = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.status    = static_cast<TriggerStatusType>(status);
		int severity       = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.severity  = static_cast<TriggerSeverityType>(severity);
		trigInfo.lastChangeTime.tv_sec = 
		  GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.lastChangeTime.tv_nsec =
		  GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.hostId    = GET_UINT64_FROM_GRP(itemGroup, idx++);
		trigInfo.hostName  = GET_STRING_FROM_GRP(itemGroup, idx++);
		trigInfo.brief     = GET_STRING_FROM_GRP(itemGroup, idx++);
	}
}

void DBClientHatohol::setTriggerInfoList(const TriggerInfoList &triggerInfoList,
                                       uint32_t serverId)
{
	DBAgentDeleteArg deleteArg;
	deleteArg.tableName = TABLE_NAME_TRIGGERS;
	deleteArg.condition =
	  StringUtils::sprintf("%s=%u",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
	    serverId);

	TriggerInfoListConstIterator it = triggerInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(deleteArg);
		for (; it != triggerInfoList.end(); ++it)
			addTriggerInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

int DBClientHatohol::getLastChangeTimeOfTrigger(uint32_t serverId)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;
	string stmt = StringUtils::sprintf("max(%s)", 
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName);
	arg.statements.push_back(stmt);
	arg.columnTypes.push_back(
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].type);
	arg.condition = StringUtils::sprintf("%s=%u",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
	    serverId);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// get the result
	if (arg.dataTable->getNumberOfRows() == 0)
		return 0;

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	const ItemData *lastTime = (*grpList.begin())->getItemAt(0);
	return ItemDataUtils::getInt(lastTime);
}

void DBClientHatohol::addEventInfo(EventInfo *eventInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addEventInfoBare(*eventInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addEventInfoList(const EventInfoList &eventInfoList)
{
	EventInfoListConstIterator it = eventInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != eventInfoList.end(); ++it)
			addEventInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::getEventInfoList(EventInfoList &eventInfoList)
{
	const ColumnDef &eventsServerId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID];
	const ColumnDef &eventsId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_ID];
	const ColumnDef &eventsTimeSec =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_SEC];
	const ColumnDef &eventsTimeNs =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_NS];
	const ColumnDef &eventsEventValue = 
	  COLUMN_DEF_EVENTS[IDX_EVENTS_EVENT_TYPE];
	const ColumnDef &eventsTriggerId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_TRIGGER_ID];

	const ColumnDef &triggersServerId =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID];
	const ColumnDef &triggersTriggerId =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID];
	const ColumnDef &triggersStatus =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_STATUS];
	const ColumnDef &triggersSeverity =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY];
	const ColumnDef &triggersHostId =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID];
	const ColumnDef &triggersHostName =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOSTNAME];
	const ColumnDef &triggersBrief =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_BRIEF];

	DBAgentSelectExArg arg;

	// Tables
	const static char *VAR_EVENTS = "e";
	const static char *VAR_TRIGGERS = "t";
	arg.tableName = StringUtils::sprintf(
	  " %s %s inner join %s %s on %s.%s=%s.%s",
	  TABLE_NAME_EVENTS, VAR_EVENTS,
	  TABLE_NAME_TRIGGERS, VAR_TRIGGERS,
	  VAR_EVENTS, eventsTriggerId.columnName,
	  VAR_TRIGGERS, triggersTriggerId.columnName);

	// Columns
	arg.pushColumn(eventsServerId,   VAR_EVENTS);
	arg.pushColumn(eventsId,         VAR_EVENTS);
	arg.pushColumn(eventsTimeSec,    VAR_EVENTS);
	arg.pushColumn(eventsTimeNs,     VAR_EVENTS);
	arg.pushColumn(eventsEventValue, VAR_EVENTS);
	arg.pushColumn(eventsTriggerId,  VAR_EVENTS);

	arg.pushColumn(triggersStatus,   VAR_TRIGGERS);
	arg.pushColumn(triggersSeverity, VAR_TRIGGERS);
	arg.pushColumn(triggersHostId,   VAR_TRIGGERS);
	arg.pushColumn(triggersHostName, VAR_TRIGGERS);
	arg.pushColumn(triggersBrief,    VAR_TRIGGERS);

	// Condition
	arg.condition = StringUtils::sprintf(
	  "%s.%s=%s.%s", 
	  VAR_EVENTS, eventsServerId.columnName,
	  VAR_TRIGGERS, triggersServerId.columnName);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		eventInfoList.push_back(EventInfo());
		EventInfo &eventInfo = eventInfoList.back();

		eventInfo.serverId   = GET_INT_FROM_GRP(itemGroup, idx++);
		eventInfo.id         = GET_UINT64_FROM_GRP(itemGroup, idx++);
		eventInfo.time.tv_sec  = GET_INT_FROM_GRP(itemGroup, idx++);
		eventInfo.time.tv_nsec = GET_INT_FROM_GRP(itemGroup, idx++);
		int type             = GET_INT_FROM_GRP(itemGroup, idx++);
		eventInfo.type       = static_cast<EventType>(type);
		eventInfo.triggerId  = GET_UINT64_FROM_GRP(itemGroup, idx++);

		int status          = GET_INT_FROM_GRP(itemGroup, idx++);
		eventInfo.status    = static_cast<TriggerStatusType>(status);
		int severity        = GET_INT_FROM_GRP(itemGroup, idx++);
		eventInfo.severity  = static_cast<TriggerSeverityType>(severity);
		eventInfo.hostId    = GET_UINT64_FROM_GRP(itemGroup, idx++);
		eventInfo.hostName  = GET_STRING_FROM_GRP(itemGroup, idx++);
		eventInfo.brief     = GET_STRING_FROM_GRP(itemGroup, idx++);
	}
}

void DBClientHatohol::setEventInfoList(const EventInfoList &eventInfoList,
                                     uint32_t serverId)
{
	DBAgentDeleteArg deleteArg;
	deleteArg.tableName = TABLE_NAME_EVENTS;
	deleteArg.condition =
	  StringUtils::sprintf("%s=%u",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName,
	    serverId);

	EventInfoListConstIterator it = eventInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(deleteArg);
		for (; it != eventInfoList.end(); ++it)
			addEventInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

uint64_t DBClientHatohol::getLastEventId(uint32_t serverId)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_EVENTS;
	string stmt = StringUtils::sprintf("max(%s)", 
	    COLUMN_DEF_EVENTS[IDX_EVENTS_ID].columnName);
	arg.statements.push_back(stmt);
	arg.columnTypes.push_back(COLUMN_DEF_EVENTS[IDX_EVENTS_ID].type);
	arg.condition = StringUtils::sprintf("%s=%u",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName, serverId);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// get the result
	if (arg.dataTable->getNumberOfRows() == 0)
		return EVENT_NOT_FOUND;

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	const ItemData *lastId = (*grpList.begin())->getItemAt(0);
	return ItemDataUtils::getUint64(lastId);
}

void DBClientHatohol::addItemInfo(ItemInfo *itemInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItemInfoBare(*itemInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addItemInfoList(const ItemInfoList &itemInfoList)
{
	ItemInfoListConstIterator it = itemInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != itemInfoList.end(); ++it)
			addItemInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::getItemInfoList(ItemInfoList &itemInfoList)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_ITEMS;
	arg.columnDefs = COLUMN_DEF_ITEMS;
	arg.columnIndexes.push_back(IDX_ITEMS_SERVER_ID);
	arg.columnIndexes.push_back(IDX_ITEMS_ID);
	arg.columnIndexes.push_back(IDX_ITEMS_HOST_ID);
	arg.columnIndexes.push_back(IDX_ITEMS_BRIEF);
	arg.columnIndexes.push_back(IDX_ITEMS_LAST_VALUE_TIME_SEC);
	arg.columnIndexes.push_back(IDX_ITEMS_LAST_VALUE_TIME_NS);
	arg.columnIndexes.push_back(IDX_ITEMS_LAST_VALUE);
	arg.columnIndexes.push_back(IDX_ITEMS_PREV_VALUE);
	arg.columnIndexes.push_back(IDX_ITEMS_ITEM_GROUP_NAME);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		itemInfoList.push_back(ItemInfo());
		ItemInfo &itemInfo = itemInfoList.back();

		itemInfo.serverId  = GET_INT_FROM_GRP(itemGroup, idx++);
		itemInfo.id        = GET_UINT64_FROM_GRP(itemGroup, idx++);
		itemInfo.hostId    = GET_UINT64_FROM_GRP(itemGroup, idx++);
		itemInfo.brief     = GET_STRING_FROM_GRP(itemGroup, idx++);
		itemInfo.lastValueTime.tv_sec = 
		  GET_INT_FROM_GRP(itemGroup, idx++);
		itemInfo.lastValueTime.tv_nsec =
		  GET_INT_FROM_GRP(itemGroup, idx++);
		itemInfo.lastValue = GET_STRING_FROM_GRP(itemGroup, idx++);
		itemInfo.prevValue = GET_STRING_FROM_GRP(itemGroup, idx++);
		itemInfo.itemGroupName = GET_STRING_FROM_GRP(itemGroup, idx++);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientHatohol::resetDBInitializedFlags(void)
{
	PrivateContext::initialized = false;
}

void DBClientHatohol::prepareSetupFunction(void)
{
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_TRIGGERS,
		NUM_COLUMNS_TRIGGERS,
		COLUMN_DEF_TRIGGERS,
	}, {
		TABLE_NAME_EVENTS,
		NUM_COLUMNS_EVENTS,
		COLUMN_DEF_EVENTS,
	}, {
		TABLE_NAME_ITEMS,
		NUM_COLUMNS_ITEMS,
		COLUMN_DEF_ITEMS,
	}
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		HATOHOL_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
	};

	DBAgent::addSetupFunction(DB_DOMAIN_ID_HATOHOL,
	                          dbSetupFunc, (void *)&DB_SETUP_FUNC_ARG);
}

void DBClientHatohol::addTriggerInfoBare(const TriggerInfo &triggerInfo)
{
	string condition = StringUtils::sprintf
	  ("server_id=%d and id=%"PRIu64, triggerInfo.serverId, triggerInfo.id);
	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_TRIGGERS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_TRIGGERS;
		arg.numColumns = NUM_COLUMNS_TRIGGERS;
		arg.columnDefs = COLUMN_DEF_TRIGGERS;
		row->ADD_NEW_ITEM(Int, triggerInfo.serverId);
		row->ADD_NEW_ITEM(Uint64, triggerInfo.id);
		row->ADD_NEW_ITEM(Int, triggerInfo.status);
		row->ADD_NEW_ITEM(Int, triggerInfo.severity),
		row->ADD_NEW_ITEM(Int, triggerInfo.lastChangeTime.tv_sec); 
		row->ADD_NEW_ITEM(Int, triggerInfo.lastChangeTime.tv_nsec); 
		row->ADD_NEW_ITEM(Uint64, triggerInfo.hostId);
		row->ADD_NEW_ITEM(String, triggerInfo.hostName);
		row->ADD_NEW_ITEM(String, triggerInfo.brief);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_TRIGGERS;
		arg.columnDefs = COLUMN_DEF_TRIGGERS;

		row->ADD_NEW_ITEM(Int, triggerInfo.serverId);
		arg.columnIndexes.push_back(IDX_TRIGGERS_SERVER_ID);

		row->ADD_NEW_ITEM(Int, triggerInfo.status);
		arg.columnIndexes.push_back(IDX_TRIGGERS_STATUS);

		row->ADD_NEW_ITEM(Int, triggerInfo.severity);
		arg.columnIndexes.push_back(IDX_TRIGGERS_SEVERITY);

		row->ADD_NEW_ITEM(Int, triggerInfo.lastChangeTime.tv_sec); 
		arg.columnIndexes.push_back
		  (IDX_TRIGGERS_LAST_CHANGE_TIME_SEC);

		row->ADD_NEW_ITEM(Int, triggerInfo.lastChangeTime.tv_nsec); 
		arg.columnIndexes.push_back
		  (IDX_TRIGGERS_LAST_CHANGE_TIME_NS);

		row->ADD_NEW_ITEM(Uint64, triggerInfo.hostId);
		arg.columnIndexes.push_back(IDX_TRIGGERS_HOST_ID);

		row->ADD_NEW_ITEM(String, triggerInfo.hostName);
		arg.columnIndexes.push_back(IDX_TRIGGERS_HOSTNAME);

		row->ADD_NEW_ITEM(String, triggerInfo.brief);
		arg.columnIndexes.push_back(IDX_TRIGGERS_BRIEF);
		arg.row = row;
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addEventInfoBare(const EventInfo &eventInfo)
{
	string condition = StringUtils::sprintf
	  ("server_id=%d and id=%"PRIu64, eventInfo.serverId, eventInfo.id);
	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_EVENTS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_EVENTS;
		arg.numColumns = NUM_COLUMNS_EVENTS;
		arg.columnDefs = COLUMN_DEF_EVENTS;
		row->ADD_NEW_ITEM(Int, eventInfo.serverId),
		row->ADD_NEW_ITEM(Uint64, eventInfo.id);
		row->ADD_NEW_ITEM(Int, eventInfo.time.tv_sec); 
		row->ADD_NEW_ITEM(Int, eventInfo.time.tv_nsec); 
		row->ADD_NEW_ITEM(Int, eventInfo.type);
		row->ADD_NEW_ITEM(Uint64, eventInfo.triggerId);
		row->ADD_NEW_ITEM(Int, eventInfo.status);
		row->ADD_NEW_ITEM(Int, eventInfo.severity);
		row->ADD_NEW_ITEM(Uint64, eventInfo.hostId);
		row->ADD_NEW_ITEM(String, eventInfo.hostName);
		row->ADD_NEW_ITEM(String, eventInfo.brief);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_EVENTS;
		arg.columnDefs = COLUMN_DEF_EVENTS;

		row->ADD_NEW_ITEM(Int, eventInfo.serverId);
		arg.columnIndexes.push_back(IDX_EVENTS_SERVER_ID);

		row->ADD_NEW_ITEM(Int, eventInfo.time.tv_sec); 
		arg.columnIndexes.push_back(IDX_EVENTS_TIME_SEC);

		row->ADD_NEW_ITEM(Int, eventInfo.time.tv_nsec); 
		arg.columnIndexes.push_back(IDX_EVENTS_TIME_NS);

		row->ADD_NEW_ITEM(Int, eventInfo.type);
		arg.columnIndexes.push_back(IDX_EVENTS_EVENT_TYPE);

		row->ADD_NEW_ITEM(Uint64, eventInfo.triggerId);
		arg.columnIndexes.push_back(IDX_EVENTS_TRIGGER_ID);

		row->ADD_NEW_ITEM(Int, eventInfo.status);
		arg.columnIndexes.push_back(IDX_EVENTS_STATUS);

		row->ADD_NEW_ITEM(Int, eventInfo.severity);
		arg.columnIndexes.push_back(IDX_EVENTS_SEVERITY);

		row->ADD_NEW_ITEM(Uint64, eventInfo.hostId);
		arg.columnIndexes.push_back(IDX_EVENTS_HOST_ID);

		row->ADD_NEW_ITEM(String, eventInfo.hostName);
		arg.columnIndexes.push_back(IDX_EVENTS_HOST_NAME);

		row->ADD_NEW_ITEM(String, eventInfo.brief);
		arg.columnIndexes.push_back(IDX_EVENTS_BRIEF);

		arg.row = row;
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addItemInfoBare(const ItemInfo &itemInfo)
{
	string condition = StringUtils::sprintf("server_id=%d and id=%"PRIu64,
	                                        itemInfo.serverId, itemInfo.id);
	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_ITEMS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_ITEMS;
		arg.numColumns = NUM_COLUMNS_ITEMS;
		arg.columnDefs = COLUMN_DEF_ITEMS;
		row->ADD_NEW_ITEM(Int,    itemInfo.serverId);
		row->ADD_NEW_ITEM(Uint64, itemInfo.id);
		row->ADD_NEW_ITEM(Uint64, itemInfo.hostId);
		row->ADD_NEW_ITEM(String, itemInfo.brief);
		row->ADD_NEW_ITEM(Int,    itemInfo.lastValueTime.tv_sec); 
		row->ADD_NEW_ITEM(Int,    itemInfo.lastValueTime.tv_nsec); 
		row->ADD_NEW_ITEM(String, itemInfo.lastValue);
		row->ADD_NEW_ITEM(String, itemInfo.prevValue);
		row->ADD_NEW_ITEM(String, itemInfo.itemGroupName);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_ITEMS;
		arg.columnDefs = COLUMN_DEF_ITEMS;

		row->ADD_NEW_ITEM(Int, itemInfo.serverId);
		arg.columnIndexes.push_back(IDX_ITEMS_SERVER_ID);

		row->ADD_NEW_ITEM(Uint64, itemInfo.id);
		arg.columnIndexes.push_back(IDX_ITEMS_ID);

		row->ADD_NEW_ITEM(Uint64, itemInfo.hostId);
		arg.columnIndexes.push_back(IDX_ITEMS_HOST_ID);

		row->ADD_NEW_ITEM(String, itemInfo.brief);
		arg.columnIndexes.push_back(IDX_ITEMS_BRIEF);

		row->ADD_NEW_ITEM(Int, itemInfo.lastValueTime.tv_sec); 
		arg.columnIndexes.push_back(IDX_ITEMS_LAST_VALUE_TIME_SEC);

		row->ADD_NEW_ITEM(Int, itemInfo.lastValueTime.tv_nsec); 
		arg.columnIndexes.push_back(IDX_ITEMS_LAST_VALUE_TIME_NS);

		row->ADD_NEW_ITEM(String, itemInfo.lastValue);
		arg.columnIndexes.push_back(IDX_ITEMS_LAST_VALUE);

		row->ADD_NEW_ITEM(String, itemInfo.prevValue);
		arg.columnIndexes.push_back(IDX_ITEMS_PREV_VALUE);

		row->ADD_NEW_ITEM(String, itemInfo.itemGroupName);
		arg.columnIndexes.push_back(IDX_ITEMS_ITEM_GROUP_NAME);

		arg.row = row;
		arg.condition = condition;
		update(arg);
	}
}
