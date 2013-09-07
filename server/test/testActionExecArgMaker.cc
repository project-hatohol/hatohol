/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include "ActionExecArgMaker.h"
#include "StringUtils.h"
using namespace mlpl;

namespace testActionExecArgMaker {

static void _assertMakeExecArgs(const string &cmdLine,
                                const char *firstExpect, ...)
{
	va_list ap;
	va_start(ap, firstExpect);
	StringVector inArgVect;
	inArgVect.push_back(firstExpect);
	while (true) {
		const char *word = va_arg(ap, const char *);
		if (!word)
			break;
		inArgVect.push_back(word);
	}
	va_end(ap);

	ActionExecArgMaker argMaker;
	StringVector argVect;
	argMaker.makeExecArg(argVect, cmdLine);
	cppcut_assert_equal(inArgVect.size(), argVect.size());
	for (size_t i = 0; i < inArgVect.size(); i++) {
		cppcut_assert_equal(inArgVect[i], argVect[i],
		                    cut_message("index: %zd", i));
	}
}
#define assertMakeExecArgs(CMD_LINE, FIRST, ...) \
cut_trace(_assertMakeExecArgs(CMD_LINE, FIRST, ##__VA_ARGS__))

static void _assertResidentCommand(const string &expectedPath,
                                   const string &expectedOption,
                                   const char *format)
{
	string path, option;
	string command = StringUtils::sprintf(
	  format, expectedPath.c_str(), expectedOption.c_str());
	ActionExecArgMaker::parseResidentCommand(command, path, option);
	cppcut_assert_equal(expectedPath, path);
	cppcut_assert_equal(expectedOption, option);
}
#define assertResidentCommand(PATH, OPT, FMT) \
cut_trace(_assertResidentCommand(PATH, OPT, FMT))

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_makeExecArg(void)
{
	assertMakeExecArgs("ls", "ls", NULL);
}

void test_makeExecArgTwo(void)
{
	assertMakeExecArgs("ls -l", "ls", "-l", NULL);
}

void test_makeExecArgQuot(void)
{
	assertMakeExecArgs("abc -n 'big bang'", "abc", "-n", "big bang", NULL);
}

void test_makeExecArgQuotEsc(void)
{
	assertMakeExecArgs("abc 'I\\'m a dog'", "abc", "I'm a dog", NULL);
}

void test_makeExecArgQuotEscDouble(void)
{
	assertMakeExecArgs("abc '\\'\\'dog'", "abc", "''dog", NULL);
}

void test_makeExecArgBackslashEscDouble(void)
{
	assertMakeExecArgs("abc '\\\\dog'", "abc", "\\dog", NULL);
}

void test_makeExecArgSimpleQuot(void)
{
	assertMakeExecArgs("I\\'m \\'p_q\\'", "I'm", "'p_q'", NULL);
}

void test_makeExecArgSimpleBackslashInQuot(void)
{
	assertMakeExecArgs("'\\ABC'", "ABC", NULL);
}

void test_parseResidentCommand(void)
{
	assertResidentCommand("/usr/lib/foo.so.1", "foo goo", "%s %s");
}

void test_parseResidentCommandNoOption(void)
{
	assertResidentCommand("/usr/lib/foo.so.1", "", "%s %s");
}

void test_parseResidentCommandSpaces(void)
{
	assertResidentCommand("/usr/lib/foo.so.1", "foo  goo", "%s   %s  ");
}

} // namespace testActionExecArgMaker
