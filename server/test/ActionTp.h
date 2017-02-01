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
#include <stdint.h>

enum {
	ACTTP_CODE_BEGIN,
	ACTTP_CODE_QUIT,
	ACTTP_REPLAY_QUIT,
	ACTTP_CODE_GET_ARG_LIST,
	ACTTP_REPLY_GET_ARG_LIST,
	ACTTP_CODE_GET_SESSION_ID,
	ACTTP_REPLY_GET_SESSION_ID,
};

static const size_t ACTTP_ARG_LIST_SIZE_LEN = 2;
static const size_t ACTTP_SESSION_ID_LEN = 36;

#define OPTION_CRASH_SOON "--crash-soon"
#define OPTION_STALL      "--stall"

