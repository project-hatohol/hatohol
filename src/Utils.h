#ifndef Utils_h
#define Utils_h

#include <cstdlib>
#include <vector>
#include <string>
#include <typeinfo>
using namespace std;

#include <StringUtils.h>
using namespace mlpl;

#include <execinfo.h>

typedef vector<string> CommandLineArg;

class Utils {
public:
	static string makeDemangledStackTraceLines(void **trace, int num);
	static void assertNotNull(void *ptr);
	static string demangle(string &str);
	static string demangle(const char *);

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

