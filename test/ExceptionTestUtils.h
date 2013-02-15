#ifndef ExceptionTestUtils_h
#define ExceptionTestUtils_h

#include <cppcutter.h>

template<class ThrowExceptionType, class CaughtExceptionType>
void _assertThrow(void)
{
	const char *brief = "foo";
	bool exceptionReceived = false;
	try {
		throw new ThrowExceptionType(brief);
	} catch (CaughtExceptionType *e) {
		exceptionReceived = true;
		cppcut_assert_equal(string(brief), string(e->what()));
		delete e;
	}
	cppcut_assert_equal(true, exceptionReceived);
}
#define assertThrow(T,C) cut_trace((_assertThrow<T,C>()))

#endif // ExceptionTestUtils_h
