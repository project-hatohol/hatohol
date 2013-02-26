/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AsuraException_h
#define AsuraException_h

#include <Logger.h>
using namespace mlpl;

#include <exception>
#include <string>
using namespace std;

#define ASURA_STACK_TRACE_SET_ENV "ASURA_EXCEPTION_STACK_TRACE"

class AsuraException : public exception
{
public:
	static const int UNKNOWN_LINE_NUMBER = -1;
	static void init(void);

	explicit AsuraException(const string &brief,
	                        const char *sourceFileName = "",
	                        int lineNumber = UNKNOWN_LINE_NUMBER);
	virtual ~AsuraException() _GLIBCXX_USE_NOEXCEPT;
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT;
	virtual const string getFancyMessage(void) const;

	const string &getSourceFileName(void) const;
	int getLineNumber(void) const;
	const string &getStackTrace(void) const;

protected:
	void saveStackTrace(void);

private:
	static bool m_saveStackTrace;

	string m_what;
	string m_sourceFileName;
	int    m_lineNumber;
	string m_stackTrace;
};

#define THROW_ASURA_EXCEPTION(FMT, ...) \
do { \
	string msg = StringUtils::sprintf(FMT, ##__VA_ARGS__); \
	throw AsuraException(msg, __FILE__, __LINE__); \
} while (0)

#endif // AsuraException_h
