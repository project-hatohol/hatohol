#ifndef ExceptionTestUtils_h
#define ExceptionTestUtils_h

#include <cppcutter.h>

template<class ThrowExceptionType, class CaughtExceptionType>
void _assertThrow(void)
{
	const char *brief = "foo";
	bool exceptionReceived = false;
	try {
		throw ThrowExceptionType(brief);
	} catch (const CaughtExceptionType &e) {
		exceptionReceived = true;
		string expected = "<:-1> ";
		expected += brief;
		expected += "\n";
		cppcut_assert_equal(expected, string(e.what()));
	}
	cppcut_assert_equal(true, exceptionReceived);
}
#define assertThrow(T,C) cut_trace((_assertThrow<T,C>()))

#endif // ExceptionTestUtils_h
