#include <cppcutter.h>
#include <sys/time.h>
#include <errno.h>
#include "Asura.h"
#include "Utils.h"
#include "Helpers.h"

namespace testUtils {

static void _assertValidateJSMethodName(const string &name, bool expect)
{
	string errMsg;
	cppcut_assert_equal(
	  expect, Utils::validateJSMethodName(name, errMsg),
	  cut_message("%s", errMsg.c_str()));
}
#define assertValidateJSMethodName(N,E) \
cut_trace(_assertValidateJSMethodName(N,E))

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getCurrTimeAsMicroSecond(void)
{
	const size_t allowedErrorInMicroSec = 100 * 1000; // 100 ms
	struct timeval tv;
	cppcut_assert_equal(0, gettimeofday(&tv, NULL),
	                    cut_message("errno: %d", errno));
	uint64_t currTime = Utils::getCurrTimeAsMicroSecond();
	uint64_t timeError = currTime;
	timeError -= tv.tv_sec * 1000 * 1000;
	timeError -= tv.tv_usec;
	cppcut_assert_equal(true, timeError < allowedErrorInMicroSec);
	if (isVerboseMode())
		cut_notify("timeError: %"PRIu64 " [us]", timeError);
}

void test_validateJSMethodName(void)
{
	assertValidateJSMethodName("IYHoooo_21", true);
}

void test_validateJSMethodNameEmpty(void)
{
	assertValidateJSMethodName("", false);
}

void test_validateJSMethodNameFromNumber(void)
{
	assertValidateJSMethodName("1foo", false);
}

void test_validateJSMethodNameWithSpace(void)
{
	assertValidateJSMethodName("o o", false);
}

void test_validateJSMethodNameWithDot(void)
{
	assertValidateJSMethodName("o.o", false);
}

void test_validateJSMethodNameWithExclamation(void)
{
	assertValidateJSMethodName("o!o", false);
}

} // namespace testUtils
