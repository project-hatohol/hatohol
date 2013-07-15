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

#ifndef ActionTp_h
#define ActionTp_h

#include <stdint.h>

enum {
	ACTTP_CODE_BEGIN,
	ACTTP_CODE_QUIT,
	ACTTP_CODE_GET_ARG_LIST,
};

enum {
	ACTTP_FLAGS_REQ = (1 << 0),
	ACTTP_FLAGS_RES = (1 << 1),
};

struct ActionTpCommHeader {
	uint32_t length; // includes this member.
	uint32_t code;   // command code;
	uint32_t flags;
};

#endif // ActionTp_h

