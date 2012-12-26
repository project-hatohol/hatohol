#ifndef utils_h
#define utils_h

#include <cstdio>
#include <vector>
#include <string>
using namespace std;

#include <stdint.h>

typedef vector<string> command_line_arg_t;

#define ASURA_P(MSG_LV, FMT, ...) \
printf(FMT, ##__VA_ARGS__)

#endif // utils_h

