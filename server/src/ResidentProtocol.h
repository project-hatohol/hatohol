/*
 * Copyright (C) 2013-2015 Project Hatohol
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
#include <cstdlib>
#include <stdint.h>
#include <SmartBuffer.h>

// definitions of packet types
enum
{
	RESIDENT_PROTO_PKT_TYPE_LAUNCHED,
	RESIDENT_PROTO_PKT_TYPE_MODULE_LOADED,
	RESIDENT_PROTO_PKT_TYPE_PARAMETERS,
	RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT,
	RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT_ACK,
};

static const uint16_t HATOHOL_SESSION_ID_LEN = 36;

// This structure should be identical to mlpl::SmartBuffer::StringHeader
struct ResidentStringHeader {
	uint32_t size;
	int32_t  offset;
} __attribute__((__packed__)) ;

// NOTE: Characters in Bytes column in this file means the following.
//  'U': Unsigned integer.
//  'S': Signed integer.
//  'V': variable length.
// * Byte order: Little endian

// [Header]
// All packets have a header with the following structure.
//
// Bytes: Description
//    4U: Packet body size (not including the header size)
//    2U: packet type defined the above

static const size_t RESIDENT_PROTO_HEADER_PKT_SIZE_LEN = 4;
static const size_t RESIDENT_PROTO_HEADER_PKT_TYPE_LEN = 2;

static const size_t RESIDENT_PROTO_HEADER_LEN =
  RESIDENT_PROTO_HEADER_PKT_SIZE_LEN + RESIDENT_PROTO_HEADER_PKT_TYPE_LEN;

// [Launched notify]
// Direction: Slave -> Master
// packet type: RESIDENT_PROTO_PKT_TYPE_LAUNCHED
// <Body> None

// [Parameters]
// Direction: Master -> Slave
// packet type: RESIDENT_PROTO_PKT_TYPE_PARAMETERS
// <Body>
// Bytes: Description
//    2U: Length of module path.
//     V: packet type defined in the above. (Not include a NULL terminator)
//    2U: Module option.
//     V: A option string. (Not include a NULL terminator)
//        This is passed as a 'arg' of init() of the module.

static const size_t RESIDENT_PROTO_PARAM_MODULE_PATH_LEN = 2;
static const size_t RESIDENT_PROTO_PARAM_MODULE_OPTION_LEN = 2;

// [Module loaded notify]
// Direction: Slave -> Master
// packet type: RESIDENT_PROTO_PKT_TYPE_MODULE_LOADED
// <Body>
//    4U: Module loaded code
enum {
	RESIDENT_PROTO_MODULE_LOADED_CODE_SUCCESS,
	RESIDENT_PROTO_MODULE_LOADED_CODE_FAIL_DLOPEN,
	RESIDENT_PROTO_MODULE_LOADED_CODE_NOT_FOUND_MOD_SYMBOL,
	RESIDENT_PROTO_MODULE_LOADED_CODE_MOD_VER_INVALID,
	RESIDENT_PROTO_MODULE_LOADED_CODE_INIT_FAILURE,
	RESIDENT_PROTO_MODULE_LOADED_CODE_NOT_FOUND_NOTIFY_EVENT,
};
static const size_t RESIDENT_PROTO_MODULE_LOADED_CODE_LEN = 4;

// [Notify Event]
// Direction: Master -> Slave
// packet type: RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT
// <Body>
// Bytes: Description
//    4U: Action ID.
//    4U: Server ID.
//    8U: Host ID.
//    8U: Time (Unix time).
//    4U: time (nanosecond part).
//    8U: Event  ID.
//    2U: Event type.
//    8U: Trigger ID.
//    2U: Trigger status.
//    2U: Trigger severity.

static const size_t RESIDENT_PROTO_EVENT_ACTION_ID_LEN        = 4;
static const size_t RESIDENT_PROTO_EVENT_SERVER_ID_LEN        = 4;
static const size_t RESIDENT_PROTO_EVENT_HOST_ID_HEADER_LEN
                      = sizeof(ResidentStringHeader);
static const size_t RESIDENT_PROTO_EVENT_EVENT_TIME_SEC_LEN   = 8;
static const size_t RESIDENT_PROTO_EVENT_EVENT_TIME_NSEC_LEN  = 4;
static const size_t RESIDENT_PROTO_EVENT_EVENT_ID_LEN         = 8;
static const size_t RESIDENT_PROTO_EVENT_EVENT_TYPE_LEN       = 2;
static const size_t RESIDENT_PROTO_EVENT_TRIGGER_ID_LEN       = 8;
static const size_t RESIDENT_PROTO_EVENT_TRIGGER_STATUS_LEN   = 2;
static const size_t RESIDENT_PROTO_EVENT_TRIGGER_SEVERITY_LEN = 2;

static const size_t RESIDENT_PROTO_EVENT_BODY_BASE_LEN =
 RESIDENT_PROTO_EVENT_ACTION_ID_LEN +
 RESIDENT_PROTO_EVENT_SERVER_ID_LEN +
 RESIDENT_PROTO_EVENT_HOST_ID_HEADER_LEN +
 RESIDENT_PROTO_EVENT_EVENT_TIME_SEC_LEN +
 RESIDENT_PROTO_EVENT_EVENT_TIME_NSEC_LEN +
 RESIDENT_PROTO_EVENT_EVENT_ID_LEN  +
 RESIDENT_PROTO_EVENT_EVENT_TYPE_LEN      +
 RESIDENT_PROTO_EVENT_TRIGGER_ID_LEN      +
 RESIDENT_PROTO_EVENT_TRIGGER_STATUS_LEN  +
 RESIDENT_PROTO_EVENT_TRIGGER_SEVERITY_LEN +
 HATOHOL_SESSION_ID_LEN;

// [Notify Event Ack]
// Direction: Slave -> Master
// packet type: RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT
// <Body>
// Bytes: Description
//    4U: result code.

static const size_t RESIDENT_PROTO_EVENT_ACK_CODE_LEN = 4;

//
// Module information
//
#define RESIDENT_MODULE_SYMBOL hatohol_resident_module
#define RESIDENT_MODULE_SYMBOL_STR "hatohol_resident_module"

// 1 -> 2: Add sessionId
static const uint16_t RESIDENT_MODULE_VERSION = 3;

struct ResidentNotifyEventArg {
	uint32_t actionId;
	uint32_t serverId;
	const char *hostIdInServer;
	timespec time;
	const char *eventId;
	uint16_t eventType;
	const char *triggerId;
	uint16_t triggerStatus;
	uint16_t triggerSeverity;
	char sessionId[HATOHOL_SESSION_ID_LEN+1]; // +1 means NULL terminator
};

struct ResidentModule {
	uint16_t moduleVersion;
	uint32_t (*init)(const char *arg);
	uint32_t (*notifyEvent)(ResidentNotifyEventArg *arg);
};

enum {
	RESIDENT_MOD_INIT_OK,
	RESIDENT_MOD_INIT_ERROR,
	RESIDENT_MOD_NOTIFY_EVENT_ACK_OK,
};

