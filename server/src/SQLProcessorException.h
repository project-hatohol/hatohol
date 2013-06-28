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

#ifndef SQLProcessorException_h
#define SQLProcessorException_h

#include "HatoholException.h"

class SQLProcessorException : public HatoholException
{
public:
	explicit SQLProcessorException(const string &brief,
	                               const char *sourceFileName = "",
	                               int lineNumber = UNKNOWN_LINE_NUMBER);
	virtual ~SQLProcessorException() _HATOHOL_NOEXCEPT;
private:
};

#define THROW_SQL_PROCESSOR_EXCEPTION(FMT, ...) \
do { \
	string msg = StringUtils::sprintf(FMT, ##__VA_ARGS__); \
	throw SQLProcessorException(msg, __FILE__, __LINE__); \
} while (0)

#endif // SQLProcessorException_h
