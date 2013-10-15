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

#ifndef HatoholError_h
#define HatoholError_h

#include <string>

enum HatoholErrorCode
{
	HTERR_OK,
	HTERR_UNINITIALIZED,
	HTERR_UNKNOWN_REASON,
	HTERR_GOT_EXCEPTION,

	// DBClientUser
	HTERR_EMPTY_USER_NAME,
	HTERR_TOO_LONG_USER_NAME,
	HTERR_INVALID_CHAR,
	HTERR_EMPTY_PASSWORD,
	HTERR_TOO_LONG_PASSWORD,
	HTERR_USER_NAME_EXIST,
	HTERR_NO_PRIVILEGE,
	HTERR_INVALID_USER_FLAGS,

	// FaceRest
	HTERR_UNSUPORTED_FORMAT,
	HTERR_NOT_FOUND_SESSION_ID,
	HTERR_NOT_FOUND_ID_IN_URL,
	HTERR_NOT_FOUND_PARAMETER,
	HTERR_INVALID_PARAMETER,
	HTERR_AUTH_FAILED,

	NUM_HATOHOL_ERROR_CODE,
};

class HatoholError {
public:
	static void init(void);
	HatoholError(const HatoholErrorCode &code = HTERR_UNINITIALIZED,
	             const std::string &optMessage = "");
	virtual ~HatoholError(void);
	const HatoholErrorCode &getCode(void) const;
	const std::string &getOptionMessage(void) const;

	bool operator==(const HatoholErrorCode &rhs) const;
	bool operator!=(const HatoholErrorCode &rhs) const;
	const HatoholError &operator=(const HatoholErrorCode &code);
private:
	// We don't use 'PrivateCotnext' (Pimpl idiom) for the performance
	// reason. We assume that this class is typically used as a return
	// type which is often propagated many times. This implies
	// the constructor may be called many times. If the constructor uses
	// 'new', the dynamic memory allocation has to be performed every time.
	// On the other hand, this implementation just needs few size of stack
	// (probably 16B on 64bit architecture).
	HatoholErrorCode m_code;
	std::string      m_optMessage;
};

#endif // HatoholError_h
