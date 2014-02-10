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

#include "HatoholError.h"
#include <map>

using namespace std;
static map<HatoholErrorCode, string> errorMessages;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void HatoholError::init(void)
{
	errorMessages[HTERR_OK] = "OK.";
	errorMessages[HTERR_UNINITIALIZED] =
	  "Uninitialized (This is probably a bug).";
	errorMessages[HTERR_UNKNOWN_REASON]  = "Unknown reason.";
	errorMessages[HTERR_NOT_IMPLEMENTED] = "Not implemented.";
	errorMessages[HTERR_GOT_EXCEPTION]   = "Got exception.";
	errorMessages[HTERR_INVALID_USER]    = "Invalid user.";

	// DBClientConfig
	errorMessages[HTERR_INVALID_MONITORING_SYSTEM_TYPE] =
	  "Invalid monitoring system type.";
	errorMessages[HTERR_INVALID_PORT_NUMBER] = "Invalid port number.";
	errorMessages[HTERR_INVALID_IP_ADDRESS]  = "Invalid IP address.";
	errorMessages[HTERR_INVALID_HOST_NAME]   = "Invalid host name.";
	errorMessages[HTERR_NO_IP_ADDRESS_AND_HOST_NAME] =
	  "No IP address and host name.";
	errorMessages[HTERR_NOT_FOUND_SERVER_ID] = "Not found server ID.";

	// DBClientUser
	errorMessages[HTERR_EMPTY_USER_NAME]      = "Empty user name.";
	errorMessages[HTERR_TOO_LONG_USER_NAME]   = "Too long user name.";
	errorMessages[HTERR_INVALID_CHAR]         = "Invalid character.";
	errorMessages[HTERR_EMPTY_PASSWORD]       = "Password is empty.";
	errorMessages[HTERR_TOO_LONG_PASSWORD]    = "Too long password.";
	errorMessages[HTERR_USER_NAME_EXIST]      = "The user name exists.";
	errorMessages[HTERR_NO_PRIVILEGE]         = "No privilege.";
	errorMessages[HTERR_INVALID_USER_FLAGS]   = "Invalid user flags.";
	errorMessages[HTERR_NOT_FOUND_USER_ID]    = "Not found user ID.";
	errorMessages[HTERR_EMPTY_USER_ROLE_NAME] = "Empty user role name.";
	errorMessages[HTERR_TOO_LONG_USER_ROLE_NAME] =
	  "Too long User role name.";
	errorMessages[HTERR_USER_ROLE_NAME_OR_FLAGS_EXIST] =
	  "The user role name or the flags exists.";
	errorMessages[HTERR_NOT_FOUND_USER_ROLE_ID] =
	  "Not found user role ID.";

	// DBClientHatohol
	errorMessages[HTERR_NOT_FOUND_SORT_ORDER] = "Not found sort order.";

	// DBClientAction
	errorMessages[HTERR_DELETE_IMCOMPLETE] = "Deletion imcpleted.";

	// FaceRest
	errorMessages[HTERR_UNSUPORTED_FORMAT]    = "Unsupported format.";
	errorMessages[HTERR_NOT_FOUND_SESSION_ID] = "Not found session ID.";
	errorMessages[HTERR_NOT_FOUND_ID_IN_URL]  = "Not found ID in the URL.";
	errorMessages[HTERR_NOT_FOUND_PARAMETER]  = "Not found parametr.";
	errorMessages[HTERR_INVALID_PARAMETER]    = "Invalid parameter.";
	errorMessages[HTERR_AUTH_FAILED]          = "Authentication failed.";
	errorMessages[HTERR_NOT_TEST_MODE]        = "Not test mode.";

	// OTHER
	errorMessages[HTERR_ERROR_TEST] = "Error test.";
}

HatoholError::HatoholError(const HatoholErrorCode &code,
                           const string &optMessage)
: m_code(code)
{
	if (!optMessage.empty())
		m_optMessage = optMessage;
}

HatoholError::~HatoholError(void)
{
}

const HatoholErrorCode &HatoholError::getCode(void) const
{
	return m_code;
}

const string &HatoholError::getOptionMessage(void) const
{
	return m_optMessage;
}

bool HatoholError::operator==(const HatoholErrorCode &rhs) const
{
	return m_code == rhs;
}

bool HatoholError::operator!=(const HatoholErrorCode &rhs) const
{
	return m_code != rhs;
}

const HatoholError &HatoholError::operator=(const HatoholErrorCode &code)
{
	m_code = code;
	m_optMessage.clear();
	return *this;
}
