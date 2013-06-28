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

#ifndef HatoholException_h
#define HatoholException_h

#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <exception>
#include <string>
using namespace std;

#ifdef _GLIBCXX_USE_NOEXCEPT
#define _HATOHOL_NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#else
#define _HATOHOL_NOEXCEPT throw()
#endif

#define HATOHOL_STACK_TRACE_SET_ENV "HATOHOL_EXCEPTION_STACK_TRACE"

class HatoholException : public exception
{
public:
	static const int UNKNOWN_LINE_NUMBER = -1;
	static void init(void);

	explicit HatoholException(const string &brief,
	                        const char *sourceFileName = "",
	                        int lineNumber = UNKNOWN_LINE_NUMBER);
	virtual ~HatoholException() _HATOHOL_NOEXCEPT;
	virtual const char* what() const _HATOHOL_NOEXCEPT;
	virtual const string getFancyMessage(void) const;

	const string &getSourceFileName(void) const;
	int getLineNumber(void) const;
	const string &getStackTrace(void) const;

protected:
	void saveStackTrace(void);
	void setBrief(const string &brief);

private:
	static bool m_saveStackTrace;

	string m_what;
	mutable string m_whatCache;
	string m_sourceFileName;
	int    m_lineNumber;
	string m_stackTrace;
};

#define THROW_HATOHOL_EXCEPTION(FMT, ...) \
do { \
	string msg = StringUtils::sprintf(FMT, ##__VA_ARGS__); \
	throw HatoholException(msg, __FILE__, __LINE__); \
} while (0)

#define HATOHOL_ASSERT(COND, FMT, ...) \
do { \
	if (!(COND)) { \
		THROW_HATOHOL_EXCEPTION("ASSERTION failed: [%s] : " FMT, #COND, ##__VA_ARGS__); \
	} \
} while (0)
#endif // HatoholException_h
