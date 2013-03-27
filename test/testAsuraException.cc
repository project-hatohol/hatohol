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

void test_asuraAssertPass(void)
{
	bool gotException = false;
	int a = 1;
	try {
		ASURA_ASSERT(a == 1, "a == 1 @ %s", __func__);
	} catch (const AsuraException &e) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

void test_asuraAssertFail(void)
{
	bool gotException = false;
	int a = 2;
	try {
		ASURA_ASSERT(a == 1, "a = %d", a);
	} catch (const AsuraException &e) {
		cut_notify("<<MSG>>\n%s", e.getFancyMessage().c_str());
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

} // namespace testAsuraException
