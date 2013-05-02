/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef Utils_h
#define Utils_h

#include <cstdlib>
#include <vector>
#include <string>
#include <typeinfo>
#include <inttypes.h>
using namespace std;

#include <StringUtils.h>
using namespace mlpl;

#include <execinfo.h>

class FormulaElement;

typedef vector<string> CommandLineArg;

class Utils {
public:
	static string makeDemangledStackTraceLines(void **trace, int num);
	static void assertNotNull(const void *ptr);
	static string demangle(string &str);
	static string demangle(const char *);
	static void showTreeInfo(FormulaElement *formulaElement, int fd = 1,
	                         bool fromRoot = true, int maxNumElem = -1,
	                         int currNum = 0, int depth = 0);
	static uint64_t getCurrTimeAsMicroSecond(void);

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

#endif // Utils_h

