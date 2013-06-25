#include <cppcutter.h>
#include "HatoholException.h"
#include "ExceptionTestUtils.h"

namespace testHatoholException {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_throw(void)
{
	assertThrow(HatoholException, HatoholException);
}

void test_throwAsException(void)
{
	assertThrow(HatoholException, exception);
}

void test_hatoholAssertPass(void)
{
	bool gotException = false;
	int a = 1;
	try {
		HATOHOL_ASSERT(a == 1, "a == 1 @ %s", __func__);
	} catch (const HatoholException &e) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

void test_hatoholAssertFail(void)
{
	bool gotException = false;
	int a = 2;
	try {
		HATOHOL_ASSERT(a == 1, "a = %d", a);
	} catch (const HatoholException &e) {
		cut_notify("<<MSG>>\n%s", e.getFancyMessage().c_str());
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

} // namespace testHatoholException
