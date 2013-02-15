#include <cppcutter.h>
#include "SQLProcessorException.h"
#include "ExceptionTestUtils.h"

namespace testSQLProcessorException {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_throw(void)
{
	assertThrow(SQLProcessorException);
}

void test_throwAsException(void)
{
	assertThrow(exception);
}

} // namespace testSQLProcessorException
