#ifndef Helpers_h
#define Helpers_h

#include <StringUtils.h>
using namespace mlpl;

void _assertStringVector(StringVector &expected, StringVector &actual);
#define assertStringVector(E,A) cut_trace(_assertStringVector(E,A))

#endif // Helpers_h
