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

#ifndef HatoholError_h
#define HatoholError_h

#include <string>
#include <map>

enum HatoholErrorCode
{
	HTERR_OK,
	HTERR_UNINITIALIZED,
	HTERR_UNKNOWN_REASON,
	HTERR_NOT_IMPLEMENTED,
	HTERR_GOT_EXCEPTION,
	HTERR_INVALID_USER,

	// DBClient
	HTERR_NOT_FOUND_TARGET_RECORD,

	// DBClientConfig
	HTERR_INVALID_MONITORING_SYSTEM_TYPE,
	HTERR_INVALID_PORT_NUMBER,
	HTERR_INVALID_IP_ADDRESS,
	HTERR_INVALID_HOST_NAME,
	HTERR_INVALID_ARM_PLUGIN_TYPE,
	HTERR_INVALID_ARM_PLUGIN_NAME,
	HTERR_DUPLICATED_ARM_PLUGIN_NAME,
	HTERR_INVALID_ARM_PLUGIN_PATH,
	HTERR_NO_IP_ADDRESS_AND_HOST_NAME,
	HTERR_INVALID_ISSUE_TRACKER_TYPE,
	HTERR_NO_ISSUE_TRACKER_LOCATION,

	// DBClientUser
	HTERR_EMPTY_USER_NAME,
	HTERR_TOO_LONG_USER_NAME,
	HTERR_INVALID_CHAR,
	HTERR_EMPTY_PASSWORD,
	HTERR_TOO_LONG_PASSWORD,
	HTERR_USER_NAME_EXIST,
	HTERR_NO_PRIVILEGE,
	HTERR_INVALID_PRIVILEGE_FLAGS,
	HTERR_EMPTY_USER_ROLE_NAME,
	HTERR_TOO_LONG_USER_ROLE_NAME,
	HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,

	// DBClientHatohol
	HTERR_OFFSET_WITHOUT_LIMIT,
	HTERR_NOT_FOUND_SORT_ORDER,

	// DBClientAction
	HTERR_DELETE_INCOMPLETE,

	// ChildProcessManager
	HTERR_INVALID_ARGS,
	HTERR_FAILED_TO_SPAWN,

	// FaceRest
	HTERR_UNSUPPORTED_FORMAT,
	HTERR_NOT_FOUND_SESSION_ID,
	HTERR_NOT_FOUND_ID_IN_URL,
	HTERR_NOT_FOUND_PARAMETER,
	HTERR_INVALID_PARAMETER,
	HTERR_AUTH_FAILED,
	HTERR_NOT_TEST_MODE,

	// VirtualDataStore
	HTERR_FAILED_TO_CREATE_DATA_STORE,
	HTERR_FAILED_TO_REGIST_DATA_STORE,
	HTERR_FAILED_TO_STOP_DATA_STORE,

	// OTHER
	HTERR_ERROR_TEST,
	HTERR_ERROR_TEST_WITHOUT_MESSAGE,

	NUM_HATOHOL_ERROR_CODE,
};

#define DEFINE_ERR(SUFFIX, MESSAGE) \
defineError(HTERR_##SUFFIX, "HTERR_"#SUFFIX, MESSAGE);

class HatoholError {
public:
	static void init(void);
	static void defineError(const HatoholErrorCode errorCode,
				const std::string &errorCodeName,
				const std::string &errorMessage);
	static const std::map<HatoholErrorCode, std::string>
	  &getCodeNames(void);
	HatoholError(const HatoholErrorCode &code = HTERR_UNINITIALIZED,
	             const std::string &optMessage = "");
	virtual ~HatoholError(void);
	const HatoholErrorCode &getCode(void) const;
	const std::string &getCodeName(void) const;
	const std::string &getMessage(void) const;
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
