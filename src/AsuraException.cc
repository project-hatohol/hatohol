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

#include <cstdlib>
#include "AsuraException.h"
#include "Utils.h"

bool AsuraException::m_saveStackTrace = false;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void AsuraException::init(void)
{
	char *env = getenv(ASURA_STACK_TRACE_SET_ENV);
	if (!env)
		return;
	if (string("1") == env)
		m_saveStackTrace = true;
}

AsuraException::AsuraException(const string &brief,
                               const char *sourceFileName, int lineNumber)
: m_what(brief),
  m_sourceFileName(sourceFileName),
  m_lineNumber(lineNumber)
{
	if (m_saveStackTrace)
		saveStackTrace();
}

AsuraException::~AsuraException() _GLIBCXX_USE_NOEXCEPT
{
}

const char* AsuraException::what() const _GLIBCXX_USE_NOEXCEPT
{
	return m_what.c_str();
}

const string &AsuraException::getSourceFileName(void) const
{
	return m_sourceFileName;
}

int AsuraException::getLineNumber(void) const
{
	return m_lineNumber;
}

const string &AsuraException::getStackTrace(void) const
{
	return m_stackTrace;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void AsuraException::saveStackTrace(void)
{
	void *trace[128];
	int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
	m_stackTrace = Utils::makeDemangledStackTraceLines(trace, n);
}
