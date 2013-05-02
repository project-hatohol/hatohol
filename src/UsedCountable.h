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

#ifndef UsedCountable_h
#define UsedCountable_h

class UsedCountable {
public:
	void ref(void) const;
	void unref(void) const;
	int getUsedCount(void) const;

protected:
	UsedCountable(int initialUsedCount = 1);
	virtual ~UsedCountable();

private:
	mutable volatile int m_usedCount;
};

#endif // UsedCountable_h
