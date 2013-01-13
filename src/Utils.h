#ifndef Utils_h
#define Utils_h

#include <vector>
#include <string>
using namespace std;

#include <StringUtils.h>
using namespace mlpl;

typedef vector<string> CommandLineArg;
#define AMSG(fmt, ...) \
StringUtils::sprintf("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#endif // Utils_h

