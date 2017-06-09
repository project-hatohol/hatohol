/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <Logger.h>
#include <StringUtils.h>
#include <exception>
#include <string>
#include "HatoholError.h"

#ifdef _GLIBCXX_USE_NOEXCEPT
#define _HATOHOL_NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#else
#define _HATOHOL_NOEXCEPT throw()
#endif

#define HATOHOL_STACK_TRACE_SET_ENV "HATOHOL_EXCEPTION_STACK_TRACE"
#define HATOHOL_EXCEPTION_ABORT_ENV "HATOHOL_EXCEPTION_ABORT"

class HatoholException : public std::exception
{
public:
	static void init(void);

	explicit HatoholException(const HatoholErrorCode errCode,
	                          const std::string &brief,
	                          const std::string &sourceFileName = "",
	                          const int &lineNumber = UNKNOWN_LINE_NUMBER);
	explicit HatoholException(const std::string &brief,
	                          const std::string &sourceFileName = "",
	                          const int &lineNumber = UNKNOWN_LINE_NUMBER);
	virtual ~HatoholException() _HATOHOL_NOEXCEPT;
	virtual const char* what() const _HATOHOL_NOEXCEPT;
	virtual std::string getFancyMessage(void) const;

	HatoholErrorCode getErrCode() const;

	const std::string &getSourceFileName(void) const;
	int getLineNumber(void) const;
	const std::string &getStackTrace(void) const;

protected:
	static const int UNKNOWN_LINE_NUMBER;

	void saveStackTrace(void);
	void setBrief(const std::string &brief);

private:
	static bool m_saveStackTrace;
	static bool m_abort;

	std::string m_what;
	mutable std::string m_whatCache;
	std::string m_sourceFileName;
	int         m_lineNumber;
	std::string m_stackTrace;
	HatoholErrorCode m_errCode;
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

#define THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(ERROR_CODE, FMT, ...) \
do { \
        throw HatoholException( \
	  ERROR_CODE, \
          mlpl::StringUtils::sprintf(FMT, ##__VA_ARGS__), \
	  __FILE__, __LINE__); \
 } while (0)

#define THROW_HATOHOL_EXCEPTION_IF_NOT_OK(HATOHOL_ERR) \
do {                                               \
	if (HATOHOL_ERR == HTERR_OK)               \
		break;                             \
	THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(   \
	  HATOHOL_ERR.getCode(),                   \
	  "%s : %s",                               \
	  HATOHOL_ERR.getMessage().c_str(),        \
	  HATOHOL_ERR.getOptionMessage().c_str()); \
} while (0)

class ExceptionCatchable {
public:
	/**
	 * Exectute this as a functor in the try-catch block.
	 *
	 * @return
	 * true if the functor throws any exception.
	 * Otherwise, false is returned.
	 */
	bool exec(void);

protected:
	virtual void operator ()(void) = 0;
	virtual void onCaught(const HatoholException &e);
	virtual void onCaught(const std::exception &e);
	virtual void onCaught(void);
};

