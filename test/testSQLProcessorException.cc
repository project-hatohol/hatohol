#include <cppcutter.h>
#include "SQLProcessorException.h"

namespace testSQLProcessorException {

template<class CaughtExceptionType>
void _assertThrow(void)
{
	const char *brief = "foo";
	bool exceptionReceived = false;
	try {
		throw new SQLProcessorException(brief);
	} catch (CaughtExceptionType *e) {
		exceptionReceived = true;
		cppcut_assert_equal(string(brief), string(e->what()));
		delete e;
	}
	cppcut_assert_equal(true, exceptionReceived);
}
#define assertThrow(T) cut_trace(_assertThrow<T>())

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
