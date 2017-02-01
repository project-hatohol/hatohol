/*
 * Copyright (C) 2013 Project Hatohol
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
#include "ResidentProtocol.h"

enum ResidentTestCmdType {
	RESIDENT_TEST_CMD_GET_EVENT_INFO,
	RESIDENT_TEST_REPLY_GET_EVENT_INFO,
	RESIDENT_TEST_CMD_UNBLOCK_REPLY_NOTIFY_EVENT,
};

static const int RESIDENT_TEST_REPLY_GET_EVENT_INFO_BODY_LEN =
  sizeof(ResidentNotifyEventArg);

#define TEST_HOST_ID_STRING "Test HOST ID in server !!!"
const char *TEST_HOST_ID_REPLY_MAGIC_CODE = (const char *)0x12345678;
const char *TEST_TRIGGER_ID_REPLY_MAGIC_CODE = (const char *)0x122333;
const char *TEST_EVENT_ID_REPLY_MAGIC_CODE   = (const char *)0x76543210;

