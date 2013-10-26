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

#ifndef Utils_h
#define Utils_h

#include <cstdlib>
#include <vector>
#include <string>
#include <typeinfo>
#include <inttypes.h>
#include <glib.h>
using namespace std;

#include <StringUtils.h>
using namespace mlpl;

#include <execinfo.h>

class FormulaElement;

typedef vector<string> CommandLineArg;

static const guint INVALID_EVENT_ID = -1;

class Utils {
public:
	static void init(void);
	static string makeDemangledStackTraceLines(void **trace, int num);
	static void assertNotNull(const void *ptr);
	static string demangle(string &str);
	static string demangle(const char *);
	static void showTreeInfo(FormulaElement *formulaElement, int fd = 1,
	                         bool fromRoot = true, int maxNumElem = -1,
	                         int currNum = 0, int depth = 0);
	static uint64_t getCurrTimeAsMicroSecond(void);
	static bool isValidPort(int port, bool showErrorMsg = true);
	static string getExtension(const string &path);
	static bool validateJSMethodName(const string &name,
	                                 string &errorMessage);
	static string getSelfExeDir(void);
	static string getStringFromGIOCondition(GIOCondition condition);

	static void executeOnGLibEventLoop(
	  GSourceFunc func, gpointer data = NULL, GMainContext *context = NULL);

	/**
	 * remove a GLIB's event.
	 *
	 * @param tag A event tag.
	 * @return true if the event was successfuly removed. Otherwise false.
	 */
	static bool removeEventSourceIfNeeded(guint tag);

	/**
	 * compute a SHA256.
	 *
	 * @param data
	 * A source string to be computed.
	 *
	 * @return A SHA256 string.
	 */
	static string sha256(const string &data);

	static pid_t getThreadId(void);

protected:
	static string makeDemangledStackTraceString(string &stackTraceLine);
};

#define TRMSG(msg, fmt, ...) \
do { \
  void *trace[128]; \
  int n = backtrace(trace, sizeof(trace) / sizeof(trace[0])); \
  msg = StringUtils::sprintf("<%s:%d> " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
  msg += Utils::makeDemangledStackTraceLines(trace, n); \
} while (0)

#define TYPE_NAME(X)            typeid(X).name()
#define DEMANGLED_TYPE_NAME(X)  Utils::demangle(TYPE_NAME(X)).c_str()

#define DEFINE_AND_ASSERT(ITEM_DATA, ACTUAL_TYPE, VAR_NAME) \
	const ACTUAL_TYPE *VAR_NAME = \
	  dynamic_cast<const ACTUAL_TYPE *>(ITEM_DATA); \
	HATOHOL_ASSERT(VAR_NAME != NULL, "Failed to dynamic cast: %s -> %s", \
	             DEMANGLED_TYPE_NAME(*ITEM_DATA), #ACTUAL_TYPE); \

#endif // Utils_h

