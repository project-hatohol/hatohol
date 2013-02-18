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

class AsuraException : public exception
{
public:
	explicit AsuraException(const string &brief);
	virtual ~AsuraException() _GLIBCXX_USE_NOEXCEPT;
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT;
private:
	string m_what;
};

#define THROW_ASURA_EXCEPTION(FMT, ...) \
do { \
	string msg = StringUtils::sprintf(FMT, ##__VA_ARGS__); \
	throw new AsuraException(msg); \
} while (0)

#define THROW_ASURA_EXCEPTION_WITH_LOG(LOG_LV, FMT, ...) \
do { \
	MLPL_##LOG_LV(FMT, ##__VA_ARGS__); \
	THROW_ASURA_EXCEPTION(FMT, ##__VA_ARGS__); \
} while (0)

#endif // AsuraException_h
