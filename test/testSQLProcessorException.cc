#include <cppcutter.h>
#include "SQLProcessorException.h"
#include "ExceptionTestUtils.h"

namespace testSQLProcessorException {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_throw(void)
{
	assertThrow(SQLProcessorException, SQLProcessorException);
}

void test_throwAsAsuraException(void)
{
	assertThrow(SQLProcessorException, AsuraException);
}

void test_throwAsException(void)
{
	assertThrow(SQLProcessorException, exception);
}

} // namespace testSQLProcessorException
