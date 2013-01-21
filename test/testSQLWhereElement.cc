#include <stdexcept>
using namespace std;

#include "SQLProcessor.h"
#include <cutter.h>
#include <cppcutter.h>

namespace testSQLWhereElement {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	SQLWhereOperatorEqual op;
	cppcut_assert_equal(SQL_WHERE_OP_EQ, op.getType());
}

} // namespace testSQLWhereElement
