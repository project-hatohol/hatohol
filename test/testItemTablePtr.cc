#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include <cppcutter.h>
#include "ItemTablePtr.h"

namespace testItemTablePtr {
// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	VariableItemTablePtr tablePtr;
	cut_assert_not_null((ItemTable *)tablePtr);
}

} // namespace testItemTablePtr

