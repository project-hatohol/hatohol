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

#include "AsuraException.h"

AsuraException::AsuraException(const string &brief,
                               const char *sourceFileName, int lineNumber)
: m_what(brief),
  m_sourceFileName(sourceFileName),
  m_lineNumber(lineNumber)
{
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
