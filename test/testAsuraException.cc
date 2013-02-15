#include <cppcutter.h>
#include "AsuraException.h"
#include "ExceptionTestUtils.h"

namespace testAsuraException {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_throw(void)
{
	assertThrow(AsuraException);
}

void test_throwAsException(void)
{
	assertThrow(exception);
}

} // namespace testAsuraException
