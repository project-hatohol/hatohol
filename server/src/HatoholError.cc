/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include <StringUtils.h>

using namespace std;
using namespace mlpl;
static map<HatoholErrorCode, string> errorCodeNames;
static map<HatoholErrorCode, string> errorMessages;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void HatoholError::init(void)
{
	DEFINE_ERR(OK, "OK.");
	DEFINE_ERR(UNINITIALIZED,
		   "Uninitialized (This is probably a bug).");
	DEFINE_ERR(UNKNOWN_REASON,
		   "Unknown reason.");
	DEFINE_ERR(NOT_IMPLEMENTED,
		   "Not implemented.");
	DEFINE_ERR(GOT_EXCEPTION,
		   "Got exception.");
	DEFINE_ERR(INVALID_USER,
		   "Invalid user.");

	// DBClientConfig
	DEFINE_ERR(NOT_FOUND_TARGET_RECORD,
		   "Not found target record.");

	// DBClientConfig
	DEFINE_ERR(INVALID_MONITORING_SYSTEM_TYPE,
		   "Invalid monitoring system type.");
	DEFINE_ERR(INVALID_PORT_NUMBER,
		   "Invalid port number.");
	DEFINE_ERR(INVALID_IP_ADDRESS,
		   "Invalid IP address.");
	DEFINE_ERR(INVALID_HOST_NAME,
		   "Invalid host name.");
	DEFINE_ERR(NO_IP_ADDRESS_AND_HOST_NAME,
		   "No IP address and host name.");
	DEFINE_ERR(INVALID_ISSUE_TRACKER_TYPE,
		   "Invalid issue tracker type.");
	DEFINE_ERR(NO_ISSUE_TRACKER_LOCATION,
		   "NO issue tracker location.");

	// DBClientUser
	DEFINE_ERR(EMPTY_USER_NAME,
		   "Empty user name.");
	DEFINE_ERR(TOO_LONG_USER_NAME,
		   "Too long user name.");
	DEFINE_ERR(INVALID_CHAR,
		   "Invalid character.");
	DEFINE_ERR(EMPTY_PASSWORD,
		   "Password is empty.");
	DEFINE_ERR(TOO_LONG_PASSWORD,
		   "Too long password.");
	DEFINE_ERR(USER_NAME_EXIST,
		   "The same user name already exists.");
	DEFINE_ERR(NO_PRIVILEGE,
		   "No privilege.");
	DEFINE_ERR(INVALID_PRIVILEGE_FLAGS,
		   "Invalid privilege flags.");
	DEFINE_ERR(EMPTY_USER_ROLE_NAME,
		   "Empty user role name.");
	DEFINE_ERR(TOO_LONG_USER_ROLE_NAME,
		   "Too long User role name.");
	DEFINE_ERR(USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,
		   "The same user role name or a user role with the same "
		   "privilege already exists.");

	// DBClientHatohol
	DEFINE_ERR(OFFSET_WITHOUT_LIMIT,
		   "An offset value is specified "
		   "but no limit value is specified.");
	DEFINE_ERR(NOT_FOUND_SORT_ORDER,
		   "Not found sort order.");

	// DBClientAction
	DEFINE_ERR(DELETE_INCOMPLETE,
		   "The delete operation was incomplete.");

	// FaceRest
	DEFINE_ERR(UNSUPPORTED_FORMAT,
		   "Unsupported format.");
	DEFINE_ERR(NOT_FOUND_SESSION_ID,
		   "Not found session ID.");
	DEFINE_ERR(NOT_FOUND_ID_IN_URL,
		   "Not found ID in the URL.");
	DEFINE_ERR(NOT_FOUND_PARAMETER,
		   "Not found parameter.");
	DEFINE_ERR(INVALID_PARAMETER,
		   "Invalid parameter.");
	DEFINE_ERR(AUTH_FAILED,
		   "Authentication failed.");
	DEFINE_ERR(NOT_TEST_MODE,
		   "Not test mode.");

	// VirtualDataStore
	DEFINE_ERR(FAILED_TO_CREATE_DATA_STORE,
		   "Failed to create a DataStore object.");
	DEFINE_ERR(FAILED_TO_REGIST_DATA_STORE,
		   "Failed to regist a DataStore object.");
	DEFINE_ERR(FAILED_TO_STOP_DATA_STORE,
		   "Failed to stop a DataStore object.");

	// Incident Mangement
	DEFINE_ERR(FAILED_TO_SEND_ISSUE,
		   "Failed to send an issue to an issue tracker.");

	// OTHER
	DEFINE_ERR(ERROR_TEST,
		   "Error test.");
	DEFINE_ERR(ERROR_TEST_WITHOUT_MESSAGE,
		   "");
}

void HatoholError::defineError(const HatoholErrorCode errorCode,
			       const string &errorCodeName,
			       const string &errorMessage)
{
	errorCodeNames[errorCode] = errorCodeName;
	errorMessages[errorCode] = errorMessage;
}

const std::map<HatoholErrorCode, std::string> &HatoholError::getCodeNames(void)
{
	return errorCodeNames;
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

const std::string &HatoholError::getCodeName(void) const
{
	if (errorCodeNames.find(m_code) != errorCodeNames.end())
		return errorCodeNames[m_code];
	return StringUtils::EMPTY_STRING;
}

const std::string &HatoholError::getMessage(void) const
{
	if (errorMessages.find(m_code) != errorMessages.end())
		return errorMessages[m_code];
	return StringUtils::EMPTY_STRING;
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
