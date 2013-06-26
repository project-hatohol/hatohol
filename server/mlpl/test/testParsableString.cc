#include <cstdio>
#include <string>
#include <vector>
using namespace std;

#include <cutter.h>
#include <cppcutter.h>
#include <stdarg.h>

#include "StringUtils.h"
#include "ParsableString.h"
using namespace mlpl;

namespace testParsableString {

static vector<string> g_receivedWords;

void _assertReceivedWords(const char *first, ...)
{
	size_t index = 0;
	size_t numReceivedWords = g_receivedWords.size();
	va_list ap;
	va_start(ap, first);
	const char *expected = first;
	for (; expected; index++) {
		cppcut_assert_equal(true, index <= numReceivedWords);
		if (!expected)
			break;
		cppcut_assert_equal(expected, g_receivedWords[index].c_str(),
		                    cut_message("index: %zd", index));
		expected = va_arg(ap, const char *);
	}
	va_end(ap);
}
#define assertReceivedWords(...) cut_trace(_assertReceivedWords(__VA_ARGS__))

void assertReadWord(ParsableString &pstr, SeparatorChecker &separator,
                    const char *expectedWord, bool expectedFinish = false)
{
	g_receivedWords.clear();
	string str = pstr.readWord(separator);
	g_receivedWords.push_back(str);
	cut_assert_equal_string(expectedWord, str.c_str());
	cppcut_assert_equal(expectedFinish, pstr.finished());
}

void assertReadWord(ParsableString &pstr, const char *separators,
                    const char *expectedWord, bool expectedFinish = false)
{
	string str = pstr.readWord(separators);
	cppcut_assert_equal(string(expectedWord), str);
	cppcut_assert_equal(expectedFinish, pstr.finished());
}

void assertCount(SeparatorCheckerWithCounter &separator, char c, int expected)
{
	cppcut_assert_equal(expected, separator.getCount(c));
}

void assertForwardAndLast(SeparatorCheckerWithCounter &separator,
                          string expectedForward, char expectedLast)
{
	string &str = separator.getForwardSeparators();
	cut_assert_equal_string(expectedForward.c_str(), str.c_str());
	cppcut_assert_equal(expectedLast, separator.getLastSeparator());
}

struct SeparatorCallbackArg {
	char lastGotChar;
	int countComma;
	int countSlash;

	SeparatorCallbackArg(void)
	: lastGotChar('\0'),
	  countComma(0),
	  countSlash(0)
	{
	}
};

static void separatorCallbackCommaExplicitArg(const char separator, SeparatorCallbackArg *arg)
{
	arg->lastGotChar = separator;
	if (separator == ',')
		arg->countComma++;
	g_receivedWords.push_back(",");
}

static void separatorCallbackComma(const char separator, void *_arg)
{
	SeparatorCallbackArg *arg = static_cast<SeparatorCallbackArg *>(_arg);
	separatorCallbackCommaExplicitArg(separator, arg);
}

static void separatorCallbackSlash(const char separator, void *_arg)
{
	SeparatorCallbackArg *arg = static_cast<SeparatorCallbackArg *>(_arg);
	arg->lastGotChar = separator;
	if (separator == '/')
		arg->countSlash++;
	g_receivedWords.push_back("/");
}

struct SeparatorCallbackQuotArg {
	bool isOpen;
	SeparatorCheckerWithCallback *separatorCheckerCb;

	SeparatorCallbackQuotArg(void)
	: isOpen(false),
	  separatorCheckerCb(NULL)
	{
	}
};

static void separatorCallbackQuot(const char separator, void *_arg)
{
	SeparatorCallbackQuotArg *arg = 
	  static_cast<SeparatorCallbackQuotArg *>(_arg);
	g_receivedWords.push_back("'");
	if (!arg->isOpen) {
		arg->isOpen = true;
		arg->separatorCheckerCb->setAlternative(&ParsableString::SEPARATOR_QUOT);
	} else {
		arg->isOpen = false;
		arg->separatorCheckerCb->unsetAlternative();
	}
}

// ----------------------------------------------------------------------------
// Test cases
// ----------------------------------------------------------------------------
void test_constructorStringRef(void)
{
	string str("aaa");
	ParsableString pstr(str);
}

void test_constructorStringInstance(void)
{
	ParsableString pstr(string("aaa"));
}

void test_constructorConstCharPtr(void)
{
	ParsableString pstr("aaa");
}

void test_readWord(void)
{
	ParsableString pstr("ABC foo");
	assertReadWord(pstr, " ", "ABC");
	assertReadWord(pstr, " ", "foo", true);
}

void test_readWordManySeparators(void)
{
	ParsableString pstr("ABC   foo");
	assertReadWord(pstr, " ", "ABC");
	assertReadWord(pstr, " ", "foo", true);
}

void test_readWordHeadAndTailSeparator(void)
{
	ParsableString pstr(" ABC foo ");
	assertReadWord(pstr, " ", "ABC");
	assertReadWord(pstr, " ", "foo");
	assertReadWord(pstr, " ", "", true);
}

void test_readWordTailManySeparator(void)
{
	ParsableString pstr("ABC     ");
	assertReadWord(pstr, " ", "ABC");
	assertReadWord(pstr, " ", "", true);
}

void test_readWordAfterFinish(void)
{
	ParsableString pstr("ABC");
	assertReadWord(pstr, " ", "ABC", true);
	assertReadWord(pstr, " ", "", true);
}

void test_readWordEmptyString(void)
{
	ParsableString pstr("");
	assertReadWord(pstr, " ", "", true);
}

void test_readWordWithBuiltinChecker(void)
{
	ParsableString pstr("ABC foo,XYZ");
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "ABC");
	assertReadWord(pstr, ParsableString::SEPARATOR_COMMA, "foo");
	assertReadWord(pstr, ParsableString::SEPARATOR_COMMA, "XYZ", true);
}

void test_readWordWithMultiSeparatorChecker(void)
{
	SeparatorChecker separator(" ,/");
	ParsableString pstr("ABC foo,XYZ/stu");
	assertReadWord(pstr, separator, "ABC");
	assertReadWord(pstr, separator, "foo");
	assertReadWord(pstr, separator, "XYZ");
	assertReadWord(pstr, separator, "stu", true);
}

void test_getString(void)
{
	const char *originalStr = "ABC foo";
	ParsableString pstr(originalStr);
	const char *str = pstr.getString();
	cut_assert_equal_string(originalStr, str);
}

void test_readWordKeepParsePtr(void)
{
	ParsableString pstr("ABC foo");
	string str = pstr.readWord(" ", true);
	cut_assert_equal_string("ABC", str.c_str());
	cppcut_assert_equal(false, pstr.finished());

	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "ABC");
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "foo", true);
}

void test_setGetParsingPosition(void)
{
	ParsableString pstr("ABC foo dog");
	ParsingPosition pos = pstr.getParsingPosition();
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "ABC");
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "foo");
	pstr.setParsingPosition(pos);
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "ABC");
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "foo");
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "dog", true);
	pstr.setParsingPosition(pos);
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "ABC");
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "foo");
	assertReadWord(pstr, ParsableString::SEPARATOR_SPACE, "dog", true);
}

void test_readWordWithSeparatorCheckerWithCounter(void)
{
	SeparatorCheckerWithCounter separator(" ");
	ParsableString pstr("ABC foo  XYZ   stu");
	assertCount(separator, ' ', 0);
	assertReadWord(pstr, separator, "ABC");
	assertCount(separator, ' ', 0);
	assertReadWord(pstr, separator, "foo");
	assertCount(separator, ' ', 1);
	assertReadWord(pstr, separator, "XYZ");
	assertCount(separator, ' ', 3);
	assertReadWord(pstr, separator, "stu", true);
	assertCount(separator, ' ', 6);
}

void test_readWordWithMultiSeparatorCheckerWithCounter(void)
{
	SeparatorCheckerWithCounter separator(" ,/");
	ParsableString pstr("ABC foo ,XYZ  /stu");
	assertCount(separator, ' ', 0);
	assertReadWord(pstr, separator, "ABC");
	assertCount(separator, ' ', 0);
	assertReadWord(pstr, separator, "foo");
	assertCount(separator, ' ', 1);
	assertReadWord(pstr, separator, "XYZ");
	assertCount(separator, ' ', 2);
	assertCount(separator, ',', 1);
	assertReadWord(pstr, separator, "stu", true);
	assertCount(separator, ' ', 4);
	assertCount(separator, '/', 1);
}

void test_readWordWithSeparatorCheckerWithReset(void)
{
	SeparatorCheckerWithCounter separator(" ,/");
	ParsableString pstr("ABC/foo ,XYZ  /stu");
	assertCount(separator, ' ', 0);
	assertReadWord(pstr, separator, "ABC");
	assertCount(separator, '/', 0);
	assertReadWord(pstr, separator, "foo");
	assertCount(separator, ' ', 0);
	assertCount(separator, '/', 1);
	separator.resetCounter();
	assertCount(separator, ' ', 0);
	assertCount(separator, '/', 0);
	assertReadWord(pstr, separator, "XYZ");
	assertCount(separator, ' ', 1);
	assertCount(separator, ',', 1);
	assertReadWord(pstr, separator, "stu", true);
	assertCount(separator, ' ', 3);
	assertCount(separator, '/', 1);
}

void test_readWordWithForwardSeparators(void)
{
	SeparatorCheckerWithCounter separator(" ");
	ParsableString pstr("ABC foo  XYZ   stu");
	assertForwardAndLast(separator, "", '\0');
	assertReadWord(pstr, separator, "ABC");
	assertForwardAndLast(separator, "", ' ');
	assertReadWord(pstr, separator, "foo");
	assertForwardAndLast(separator, " ", ' ');
	assertReadWord(pstr, separator, "XYZ");
	assertForwardAndLast(separator, "  ", ' ');
	assertReadWord(pstr, separator, "stu", true);
	assertForwardAndLast(separator, "   ", '\0');
}

void test_readWordWithForwardSeparatorsMulti(void)
{
	SeparatorCheckerWithCounter separator(" ,/");
	ParsableString pstr("ABC foo ,XYZ  /stu");
	assertForwardAndLast(separator, "", '\0');
	assertReadWord(pstr, separator, "ABC");
	assertForwardAndLast(separator, "", ' ');
	assertReadWord(pstr, separator, "foo");
	assertForwardAndLast(separator, " ", ' ');
	assertReadWord(pstr, separator, "XYZ");
	assertForwardAndLast(separator, " ,", ' ');
	assertReadWord(pstr, separator, "stu", true);
	assertForwardAndLast(separator, "  /", '\0');
}

void test_readWordForwardSeparatorsMultiWithReset(void)
{
	SeparatorCheckerWithCounter separator(" ,/");
	ParsableString pstr("ABC/foo ,XYZ  /stu");
	assertForwardAndLast(separator, "", '\0');
	assertReadWord(pstr, separator, "ABC");
	assertForwardAndLast(separator, "", '/');
	assertReadWord(pstr, separator, "foo");
	assertForwardAndLast(separator, "/", ' ');
	separator.resetCounter();
	assertReadWord(pstr, separator, "XYZ");
	assertForwardAndLast(separator, " ,", ' ');
	assertReadWord(pstr, separator, "stu", true);
	assertForwardAndLast(separator, "  /", '\0');
}

void test_separatorWithCallback(void)
{
	bool ret;
	SeparatorCallbackArg arg;
	SeparatorCheckerWithCallback separator(" ,/");
	ret = separator.setCallback(',', separatorCallbackComma, &arg);
	cppcut_assert_equal(true, ret);
	ret = separator.setCallback('/', separatorCallbackSlash, &arg);
	cppcut_assert_equal(true, ret);

	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);
	ParsableString pstr("ABC/foo ,XYZ  /stu");

	assertReadWord(pstr, separator, "ABC");
	assertReceivedWords("ABC", NULL);
	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);

	assertReadWord(pstr, separator, "foo");
	assertReceivedWords("/", "foo",NULL);
	cppcut_assert_equal(1, arg.countSlash);

	assertReadWord(pstr, separator, "XYZ");
	assertReceivedWords(",", "XYZ", NULL);
	cppcut_assert_equal(1, arg.countComma);

	assertReadWord(pstr, separator, "stu", true);
	assertReceivedWords("/", "stu", NULL);
	cppcut_assert_equal(2, arg.countSlash);
}

void test_separatorWithCallbackWithMultiSeparator(void)
{
	bool ret;
	SeparatorCallbackArg arg;
	SeparatorCheckerWithCallback separator(" ,/");
	ret = separator.setCallback(',', separatorCallbackComma, &arg);
	cppcut_assert_equal(true, ret);
	ret = separator.setCallback('/', separatorCallbackSlash, &arg);
	cppcut_assert_equal(true, ret);

	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);
	ParsableString pstr("ABC/,,foo");

	assertReadWord(pstr, separator, "ABC");
	assertReceivedWords("ABC", NULL);
	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);

	assertReadWord(pstr, separator, "foo", true);
	assertReceivedWords("/", ",", ",", "foo",NULL);
	cppcut_assert_equal(1, arg.countSlash);
	cppcut_assert_equal(2, arg.countComma);
}


void test_separatorWithCallbackFromSeparator(void)
{
	bool ret;
	SeparatorCallbackArg arg;
	SeparatorCheckerWithCallback separator(" ,/");
	ret = separator.setCallback(',', separatorCallbackComma, &arg);
	cppcut_assert_equal(true, ret);
	ret = separator.setCallback('/', separatorCallbackSlash, &arg);
	cppcut_assert_equal(true, ret);

	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);
	ParsableString pstr(",ABC");

	assertReadWord(pstr, separator, "ABC", true);
	assertReceivedWords(",", "ABC", NULL);
	cppcut_assert_equal(1, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);
}

void test_separatorWithCallbackFromMultiSeparator(void)
{
	bool ret;
	SeparatorCallbackArg arg;
	SeparatorCheckerWithCallback separator(" ,/");
	ret = separator.setCallback(',', separatorCallbackComma, &arg);
	cppcut_assert_equal(true, ret);
	ret = separator.setCallback('/', separatorCallbackSlash, &arg);
	cppcut_assert_equal(true, ret);

	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);
	ParsableString pstr("/,ABC");

	assertReadWord(pstr, separator, "ABC", true);
	assertReceivedWords("/", ",", "ABC", NULL);
	cppcut_assert_equal(1, arg.countComma);
	cppcut_assert_equal(1, arg.countSlash);
}

void test_separatorWithCallbackEndWithSeparator(void)
{
	bool ret;
	SeparatorCallbackArg arg;
	SeparatorCheckerWithCallback separator(" ,/");
	ret = separator.setCallback(',', separatorCallbackComma, &arg);
	cppcut_assert_equal(true, ret);
	ret = separator.setCallback('/', separatorCallbackSlash, &arg);
	cppcut_assert_equal(true, ret);

	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);
	ParsableString pstr("ABC,");

	assertReadWord(pstr, separator, "ABC");
	assertReceivedWords("ABC", NULL);
	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);

	assertReadWord(pstr, separator, "", true);
	assertReceivedWords(",", "", NULL);
	cppcut_assert_equal(1, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);
}

void test_separatorWithCallbackEndWithMultiSeparator(void)
{
	bool ret;
	SeparatorCallbackArg arg;
	SeparatorCheckerWithCallback separator(" ,/");
	ret = separator.setCallback(',', separatorCallbackComma, &arg);
	cppcut_assert_equal(true, ret);
	ret = separator.setCallback('/', separatorCallbackSlash, &arg);
	cppcut_assert_equal(true, ret);

	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);
	ParsableString pstr("ABC,/");

	assertReadWord(pstr, separator, "ABC");
	assertReceivedWords("ABC", NULL);
	cppcut_assert_equal(0, arg.countComma);
	cppcut_assert_equal(0, arg.countSlash);

	assertReadWord(pstr, separator, "", true);
	assertReceivedWords(",", "/", "", NULL);
	cppcut_assert_equal(1, arg.countComma);
	cppcut_assert_equal(1, arg.countSlash);
}

void test_separatorWithCallbackSetBadCallback(void)
{
	bool ret;
	SeparatorCallbackArg arg;
	SeparatorCheckerWithCallback separator(" ,/");
	ret = separator.setCallback('*', separatorCallbackComma, &arg);
	cppcut_assert_equal(false, ret);
	ret = separator.setCallback('-', separatorCallbackSlash, &arg);
	cppcut_assert_equal(false, ret);
}

void test_separatorWithCallbackTemplate(void)
{
	bool ret;
	SeparatorCallbackArg arg;
	SeparatorCheckerWithCallback separator(" ,");
	ret = separator.setCallbackTempl<SeparatorCallbackArg>
	        (',', separatorCallbackCommaExplicitArg, &arg);
	cppcut_assert_equal(true, ret);

	cppcut_assert_equal(0, arg.countComma);
	ParsableString pstr("ABC,foo ,XYZ");

	assertReadWord(pstr, separator, "ABC");
	assertReceivedWords("ABC", NULL);
	cppcut_assert_equal(0, arg.countComma);

	assertReadWord(pstr, separator, "foo");
	assertReceivedWords(",", "foo", NULL);
	cppcut_assert_equal(',', arg.lastGotChar);
	cppcut_assert_equal(1, arg.countComma);

	assertReadWord(pstr, separator, "XYZ", true);
	assertReceivedWords(",", "XYZ", NULL);
	cppcut_assert_equal(',', arg.lastGotChar);
	cppcut_assert_equal(2, arg.countComma);
}

void test_readWordWithAlternative(void)
{
	ParsableString pstr("ABC foo XYZ,T S,goo ,JK");
	SeparatorChecker separator(" ");
	assertReadWord(pstr, separator, "ABC");
	separator.setAlternative(&ParsableString::SEPARATOR_COMMA);
	assertReadWord(pstr, separator, "foo XYZ");
	assertReadWord(pstr, separator, "T S");
	separator.unsetAlternative();
	assertReadWord(pstr, separator, "goo");
	assertReadWord(pstr, separator, ",JK", true);
}

void test_readWordWithAlternativeWhenNotAllow(void)
{
	ParsableString pstr("ABC foo XYZ,T S,goo ,JK");
	SeparatorChecker separator(" ", false);
	assertReadWord(pstr, separator, "ABC");
	separator.setAlternative(&ParsableString::SEPARATOR_COMMA);
	assertReadWord(pstr, separator, "foo");
	assertReadWord(pstr, separator, "XYZ,T");
	separator.unsetAlternative();
	assertReadWord(pstr, separator, "S,goo");
	assertReadWord(pstr, separator, ",JK", true);
}

void test_readWordWithAlternativeWithCallback(void)
{
	bool ret;
	ParsableString pstr("ABC='foo goo' XYZ");
	SeparatorCheckerWithCallback separator("= '");

	SeparatorCallbackQuotArg arg;
	arg.separatorCheckerCb = &separator;
	ret = separator.setCallback('\'', separatorCallbackQuot, &arg);
	cppcut_assert_equal(true, ret);

	assertReadWord(pstr, separator, "ABC");
	assertReceivedWords("ABC", NULL);

	assertReadWord(pstr, separator, "foo goo");
	assertReceivedWords("'", "foo goo", NULL);

	assertReadWord(pstr, separator, "XYZ", true);
	assertReceivedWords("'", "XYZ", NULL);
}

void test_readWordWithAddSeparators(void)
{
	ParsableString pstr("A,C foo,goo XYZ");
	SeparatorChecker separator(" ");

	assertReadWord(pstr, separator, "A,C");

	separator.addSeparator(",");
	assertReadWord(pstr, separator, "foo");
	assertReadWord(pstr, separator, "goo");
	assertReadWord(pstr, separator, "XYZ", true);
}

void test_publicMemberSEPARATOR_PARENTHESIS(void)
{
	ParsableString pstr("A(C (fo,)g oo");
	SeparatorChecker &separator = ParsableString::SEPARATOR_PARENTHESIS;
	assertReadWord(pstr, separator, "A");
	assertReadWord(pstr, separator, "C ");
	assertReadWord(pstr, separator, "fo,");
	assertReadWord(pstr, separator, "g oo", true);
}

} // namespace testParsableString
