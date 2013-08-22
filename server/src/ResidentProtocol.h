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

#ifndef ResidentProtocol_h
#define ResidentProtocol_h

#include <stdint.h>

// definitions of packet types
enum
{
	RESIDENT_PROTO_PKT_TYPE_LAUNCHED,
	RESIDENT_PROTO_PKT_TYPE_PARAMETERS,
};

// NOTE: 'V' in Bytes column in this file means variable length.

// [Header]
// All packets have a header with the following structure.
//
// Bytes: Description
//     4: Packet body size (not including the header size)
//     2: packet type defined the above

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
//     2: Length of module path.
//     V: packet type defined in the above. (Not include a NULL terminator)

static const size_t RESIDENT_PROTO_PARAM_MODULE_PATH_LEN = 2;

#endif // ResidentProtocol_h
