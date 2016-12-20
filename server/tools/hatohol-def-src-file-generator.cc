/*
 * Copyright (C) 2013-2016 Project Hatohol
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

#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <Hatohol.h>
#include <DBTablesMonitoring.h>
#include <DBTablesConfig.h>
#include <ActionManager.h>
#include <HatoholError.h>
#include <FaceRest.h>
#include <SessionManager.h>
#include <ArmStatus.h>
using namespace std;
using namespace mlpl;

enum LanguageType {
	JAVASCRIPT,
	PYTHON,
};

#define APPEND(CONTENT, FMT, ...) \
do { CONTENT += StringUtils::sprintf(FMT, ##__VA_ARGS__); } while(0)

#define ADD_LINE(SOURCE, LANG_TYPE, VAL) \
SOURCE += makeLine(LANG_TYPE, #VAL, toString(VAL))

#define DEF_LINE(SOURCE, LANG_TYPE, VAL, TYPE, ACTUAL) \
do { \
	TYPE VAL = ACTUAL; \
	ADD_LINE(SOURCE, LANG_TYPE, VAL); \
} while (0)

static const char *LGPL_V3_HEADER_C_STYLE =
"/*\n"
" * Copyright (C) 2013-2015 Project Hatohol\n"
" *\n"
" * This file is part of Hatohol.\n"
" *\n"
" * Hatohol is free software: you can redistribute it and/or modify\n"
" * it under the terms of the GNU Lesser General Public License, version 3\n"
" * as published by the Free Software Foundation.\n"
" *\n"
" * Hatohol is distributed in the hope that it will be useful,\n"
" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
" * GNU Lesser General Public License for more details.\n"
" *\n"
" * You should have received a copy of the GNU Lesser General Public\n"
" * License along with Hatohol. If not, see\n"
" * <http://www.gnu.org/licenses/>.\n"
" */\n";

static const char *LGPL_V3_HEADER_PLAIN =
"  Copyright (C) 2013-2015 Project Hatohol\n"
"\n"
"  This file is part of Hatohol.\n"
"\n"
"  Hatohol is free software: you can redistribute it and/or modify\n"
"  it under the terms of the GNU Lesser General Public License, version 3\n"
"  as published by the Free Software Foundation.\n"
"\n"
"  Hatohol is distributed in the hope that it will be useful,\n"
"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
"  GNU Lesser General Public License for more details.\n"
"\n"
"  You should have received a copy of the GNU Lesser General Public\n"
"  License along with Hatohol. If not, see\n"
"  <http://www.gnu.org/licenses/>.\n"
"";

static string toString(const int value)
{
	return StringUtils::sprintf("%d", value);
}

static string toString(const uint64_t value)
{
	return StringUtils::sprintf("%" PRIu64, value);
}

static string toString(const string &value)
{
	return StringUtils::sprintf("'%s'", value.c_str());
}

static string makeLine(LanguageType langType,
                       const string &varName, const string value)
{
	string line;
	switch(langType) {
	case JAVASCRIPT:
		line = StringUtils::sprintf("  %s: %s,",
		                            varName.c_str(), value.c_str());
		break;
	case PYTHON:
		line = StringUtils::sprintf("%s = %s",
		                            varName.c_str(), value.c_str());
		break;
	default:
		THROW_HATOHOL_EXCEPTION("Unknown language type: %d\n",
		                        langType);
	}
	line += "\n";
	return line;
}

static void makeDefSourceValues(string &s, LanguageType langType)
{
	ADD_LINE(s, langType, TRIGGER_STATUS_OK);
	ADD_LINE(s, langType, TRIGGER_STATUS_PROBLEM);
	ADD_LINE(s, langType, TRIGGER_STATUS_UNKNOWN);
	APPEND(s, "\n");

	ADD_LINE(s, langType, EVENT_TYPE_GOOD);
	ADD_LINE(s, langType, EVENT_TYPE_BAD);
	ADD_LINE(s, langType, EVENT_TYPE_NOTIFICATION);
	APPEND(s, "\n");

	ADD_LINE(s, langType, TRIGGER_SEVERITY_UNKNOWN);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_INFO);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_WARNING);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_ERROR);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_CRITICAL);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_EMERGENCY);
	APPEND(s, "\n");

	ADD_LINE(s, langType, ITEM_INFO_VALUE_TYPE_UNKNOWN);
	ADD_LINE(s, langType, ITEM_INFO_VALUE_TYPE_FLOAT);
	ADD_LINE(s, langType, ITEM_INFO_VALUE_TYPE_INTEGER);
	ADD_LINE(s, langType, ITEM_INFO_VALUE_TYPE_STRING);
	APPEND(s, "\n");

	ADD_LINE(s, langType, ACTION_USER_DEFINED);
	ADD_LINE(s, langType, ACTION_ALL);
	ADD_LINE(s, langType, ACTION_COMMAND);
	ADD_LINE(s, langType, ACTION_RESIDENT);
	ADD_LINE(s, langType, ACTION_INCIDENT_SENDER);
	APPEND(s, "\n");

	ADD_LINE(s, langType, CMP_INVALID);
	ADD_LINE(s, langType, CMP_EQ);
	ADD_LINE(s, langType, CMP_EQ_GT);
	APPEND(s, "\n");

	ADD_LINE(s, langType, MONITORING_SYSTEM_HAPI_JSON);
	ADD_LINE(s, langType, MONITORING_SYSTEM_HAPI2);
	ADD_LINE(s, langType, MONITORING_SYSTEM_UNKNOWN);
	APPEND(s, "\n");

	ADD_LINE(s, langType, INCIDENT_TRACKER_UNKNOWN);
	ADD_LINE(s, langType, INCIDENT_TRACKER_FAKE);
	ADD_LINE(s, langType, INCIDENT_TRACKER_REDMINE);
	ADD_LINE(s, langType, INCIDENT_TRACKER_HATOHOL);
	APPEND(s, "\n");

	//
	// HatoholError
	//
	map<HatoholErrorCode, string> errorNames;
	map<HatoholErrorCode, string>::iterator it;
	errorNames = HatoholError::getCodeNames();
	for (it = errorNames.begin(); it != errorNames.end(); it++) {
		HatoholErrorCode code = it->first;
		const string &name = it->second;
		if (!name.empty())
			s += makeLine(langType, name, toString(code));
	}
	APPEND(s, "\n");

	//
	// FaceRest
	//
	DEF_LINE(s, langType, FACE_REST_API_VERSION,
	         int, FaceRest::API_VERSION);
	DEF_LINE(s, langType, FACE_REST_SESSION_ID_HEADER_NAME,
	         string, FaceRest::SESSION_ID_HEADER_NAME);
	APPEND(s, "\n");

	//
	// DataQueryOption
	//
	DEF_LINE(s, langType, DATA_QUERY_OPTION_SORT_DONT_CARE,
	         DataQueryOption::SortDirection,
		 DataQueryOption::SORT_DONT_CARE);
	DEF_LINE(s, langType, DATA_QUERY_OPTION_SORT_ASCENDING,
	         DataQueryOption::SortDirection,
		 DataQueryOption::SORT_ASCENDING);
	DEF_LINE(s, langType, DATA_QUERY_OPTION_SORT_DESCENDING,
	         DataQueryOption::SortDirection,
		 DataQueryOption::SORT_DESCENDING);
	APPEND(s, "\n");

	//
	// OperationPrivilege
	//
	DEF_LINE(s, langType, ALL_PRIVILEGES, OperationPrivilegeFlag,
		 OperationPrivilege::ALL_PRIVILEGES);
	DEF_LINE(s, langType, NONE_PRIVILEGE, OperationPrivilegeFlag,
		 OperationPrivilege::NONE_PRIVILEGE);

	ADD_LINE(s, langType, OPPRVLG_CREATE_USER);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_USER);
	ADD_LINE(s, langType, OPPRVLG_DELETE_USER);
	ADD_LINE(s, langType, OPPRVLG_GET_ALL_USER);

	ADD_LINE(s, langType, OPPRVLG_CREATE_SERVER);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_SERVER);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_ALL_SERVER);
	ADD_LINE(s, langType, OPPRVLG_DELETE_SERVER);
	ADD_LINE(s, langType, OPPRVLG_DELETE_ALL_SERVER);
	ADD_LINE(s, langType, OPPRVLG_GET_ALL_SERVER);

	ADD_LINE(s, langType, OPPRVLG_CREATE_ACTION);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_ACTION);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_ALL_ACTION);
	ADD_LINE(s, langType, OPPRVLG_DELETE_ACTION);
	ADD_LINE(s, langType, OPPRVLG_DELETE_ALL_ACTION);
	ADD_LINE(s, langType, OPPRVLG_GET_ALL_ACTION);

	ADD_LINE(s, langType, OPPRVLG_CREATE_USER_ROLE);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_ALL_USER_ROLE);
	ADD_LINE(s, langType, OPPRVLG_DELETE_ALL_USER_ROLE);

	ADD_LINE(s, langType, OPPRVLG_CREATE_INCIDENT_SETTING);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_INCIDENT_SETTING);
	ADD_LINE(s, langType, OPPRVLG_DELETE_INCIDENT_SETTING);
	ADD_LINE(s, langType, OPPRVLG_GET_ALL_INCIDENT_SETTINGS);

	ADD_LINE(s, langType, OPPRVLG_GET_SYSTEM_INFO);
	ADD_LINE(s, langType, OPPRVLG_SYSTEM_OPERATION);

	ADD_LINE(s, langType, OPPRVLG_CREATE_SEVERITY_RANK);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_SEVERITY_RANK);
	ADD_LINE(s, langType, OPPRVLG_DELETE_SEVERITY_RANK);

	ADD_LINE(s, langType, NUM_OPPRVLG);
	APPEND(s, "\n");

	// Session
	DEF_LINE(s, langType, SESSION_ID_LEN,
	         int, SessionManager::SESSION_ID_LEN);
	APPEND(s, "\n");

	DEF_LINE(s, langType, ENV_NAME_SESSION_ID,
	         string, ActionManager::ENV_NAME_SESSION_ID);
	APPEND(s, "\n");

	// ArmStatus
	ADD_LINE(s, langType, ARM_WORK_STAT_INIT);
	ADD_LINE(s, langType, ARM_WORK_STAT_OK);
	ADD_LINE(s, langType, ARM_WORK_STAT_FAILURE);
}

static void makeJsDefSourceErrorMessages(string &s)
{
	map<HatoholErrorCode, string> errorNames;
	map<HatoholErrorCode, string>::iterator it;
	errorNames = HatoholError::getCodeNames();

	s += "  errorMessages: {\n";
	for (it = errorNames.begin(); it != errorNames.end(); it++) {
		HatoholErrorCode code = it->first;
		HatoholError error(code);
		const string &message = error.getMessage();
		if (message.empty())
			continue;
		string escapedMessage =
		  StringUtils::replace(message, "'", "\\'");
		string sourceCode = StringUtils::sprintf(
		  "gettext('%s')", escapedMessage.c_str());
		s += "  ";
		s += makeLine(JAVASCRIPT, toString(code), sourceCode);
	}
	s += "  },\n";
}

static string makeDefSource(LanguageType langType)
{
	string s;
	switch (langType) {
	case JAVASCRIPT:
		s += LGPL_V3_HEADER_C_STYLE;
		APPEND(s, "\n");
		APPEND(s, "var hatohol = {\n");
		makeDefSourceValues(s, langType);
		APPEND(s, "\n");
		makeJsDefSourceErrorMessages(s);
		APPEND(s, "};\n");
		APPEND(s, "\n");
		break;
	case PYTHON:
		APPEND(s, "\"\"\"\n");
		s += LGPL_V3_HEADER_PLAIN;
		APPEND(s, "\"\"\"\n");
		APPEND(s, "\n");
		makeDefSourceValues(s, langType);
		break;
	default:
		THROW_HATOHOL_EXCEPTION("Unknown language type: %d\n",
		                        langType);
	}
	return s;
}

static string makeJsDefSource(char *arg[])
{
	string s;
	s += LGPL_V3_HEADER_C_STYLE;
	APPEND(s, "\n");

	APPEND(s, "var TRIGGER_STATUS_OK      = %d\n",  TRIGGER_STATUS_OK);
	APPEND(s, "var TRIGGER_STATUS_PROBLEM = %d\n",  TRIGGER_STATUS_PROBLEM);
	APPEND(s, "\n");

	APPEND(s, "var EVENT_TYPE_GOOD         = %d\n", EVENT_TYPE_GOOD);
	APPEND(s, "var EVENT_TYPE_BAD          = %d\n", EVENT_TYPE_BAD);
	APPEND(s, "var EVENT_TYPE_NOTIFICATION = %d\n", EVENT_TYPE_NOTIFICATION);
	APPEND(s, "\n");

	APPEND(s,
	  "var TRIGGER_SEVERITY_UNKNOWN   = %d\n",  TRIGGER_SEVERITY_UNKNOWN);
	APPEND(s,
	  "var TRIGGER_SEVERITY_INFO      = %d\n",  TRIGGER_SEVERITY_INFO);
	APPEND(s,
	  "var TRIGGER_SEVERITY_WARNING   = %d\n",  TRIGGER_SEVERITY_WARNING);
	APPEND(s,
	  "var TRIGGER_SEVERITY_ERROR     = %d\n",  TRIGGER_SEVERITY_ERROR);
	APPEND(s,
	  "var TRIGGER_SEVERITY_CRITICAL  = %d\n",  TRIGGER_SEVERITY_CRITICAL);
	APPEND(s,
	  "var TRIGGER_SEVERITY_EMERGENCY = %d\n",  TRIGGER_SEVERITY_EMERGENCY);
	APPEND(s, "\n");

	APPEND(s, "var ACTION_COMMAND  = %d\n",  ACTION_COMMAND);
	APPEND(s, "var ACTION_RESIDENT = %d\n",  ACTION_RESIDENT);
	APPEND(s, "\n");

	APPEND(s, "var CMP_INVALID = %d\n",  CMP_INVALID);
	APPEND(s, "var CMP_EQ      = %d\n",  CMP_EQ);
	APPEND(s, "var CMP_EQ_GT   = %d\n",  CMP_EQ_GT);
	APPEND(s, "\n");

	return s;
}

static void printUsage(void)
{
	printf("Usage:\n");
	printf("\n");
	printf("  $ hatohol-def-src-file-generator command\n");
	printf("\n");
	printf("command:\n");
	printf("  js      : JavaScript\n");
	printf("  py      : Python\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
	hatoholInit();

	if (argc < 2) {
		printUsage();
		return EXIT_FAILURE;
	}
	string cmd(argv[1]);
	string content;
	if (cmd == "js") {
		content = makeJsDefSource(&argv[2]);
		content += "\n";
		content += makeDefSource(JAVASCRIPT);
	} else if (cmd == "py") {
		content = makeDefSource(PYTHON);
	} else {
		printf("Unknown command: %s\n", cmd.c_str());
		return EXIT_FAILURE;
	}
	printf("%s", content.c_str());

	return EXIT_SUCCESS;
}
