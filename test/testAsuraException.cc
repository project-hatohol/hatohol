#include <cppcutter.h>
#include "AsuraException.h"
#include "ExceptionTestUtils.h"

namespace testAsuraException {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_throw(void)
{
	assertThrow(AsuraException, AsuraException);
}

void test_throwAsException(void)
{
	assertThrow(AsuraException, exception);
}

} // namespace testAsuraException
