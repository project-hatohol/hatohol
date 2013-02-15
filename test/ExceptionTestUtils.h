#ifndef ExceptionTestUtils_h
#define ExceptionTestUtils_h

template<class CaughtExceptionType>
void _assertThrow(void)
{
	const char *brief = "foo";
	bool exceptionReceived = false;
	try {
		throw new AsuraException(brief);
	} catch (CaughtExceptionType *e) {
		exceptionReceived = true;
		cppcut_assert_equal(string(brief), string(e->what()));
		delete e;
	}
	cppcut_assert_equal(true, exceptionReceived);
}
#define assertThrow(T) cut_trace(_assertThrow<T>())

#endif // ExceptionTestUtils_h
