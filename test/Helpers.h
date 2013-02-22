#ifndef Helpers_h
#define Helpers_h

#include <StringUtils.h>
using namespace mlpl;

typedef pair<int,int>      IntIntPair;
typedef vector<IntIntPair> IntIntPairVector;

void _assertStringVector(StringVector &expected, StringVector &actual);
#define assertStringVector(E,A) cut_trace(_assertStringVector(E,A))

#endif // Helpers_h
