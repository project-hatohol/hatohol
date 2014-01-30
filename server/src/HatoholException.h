/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include <exception>
#include <string>

#ifdef _GLIBCXX_USE_NOEXCEPT
#define _HATOHOL_NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#else
#define _HATOHOL_NOEXCEPT throw()
#endif

#define HATOHOL_STACK_TRACE_SET_ENV "HATOHOL_EXCEPTION_STACK_TRACE"

class HatoholException : public std::exception
{
public:
	static void init(void);

	explicit HatoholException(const std::string &brief,
	                          const std::string &sourceFileName = "",
	                          const int &lineNumber = UNKNOWN_LINE_NUMBER);
	virtual ~HatoholException() _HATOHOL_NOEXCEPT;
	virtual const char* what() const _HATOHOL_NOEXCEPT;
	virtual std::string getFancyMessage(void) const;

	const std::string &getSourceFileName(void) const;
	int getLineNumber(void) const;
	const std::string &getStackTrace(void) const;

protected:
	static const int UNKNOWN_LINE_NUMBER;

	void saveStackTrace(void);
	void setBrief(const std::string &brief);

private:
	static bool m_saveStackTrace;

	std::string m_what;
	mutable std::string m_whatCache;
	std::string m_sourceFileName;
	int         m_lineNumber;
	std::string m_stackTrace;
};

#define THROW_HATOHOL_EXCEPTION(FMT, ...) \
do { \
	throw HatoholException( \
	  mlpl::StringUtils::sprintf(FMT, ##__VA_ARGS__), \
	  __FILE__, __LINE__); \
} while (0)

#define HATOHOL_ASSERT(COND, FMT, ...) \
do { \
	if (!(COND)) { \
		THROW_HATOHOL_EXCEPTION("ASSERTION failed: [%s] : " FMT, #COND, ##__VA_ARGS__); \
	} \
} while (0)
#endif // HatoholException_h
