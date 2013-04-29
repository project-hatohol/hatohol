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

#ifndef ReadWriteLock_h
#define ReadWriteLock_h

#include <glib.h>

class ReadWriteLock {
public:
	ReadWriteLock(void);
	virtual ~ReadWriteLock();

	void readLock(void) const;
	void readUnlock(void) const;
	void writeLock(void) const;
	void writeUnlock(void) const;

private:
#ifdef GLIB_VERSION_2_32
	mutable GRWLock      m_lock;
#else
	mutable GStaticMutex m_lock;
#endif // GLIB_VERSION_2_32
};

#endif // ReadWriteLock_h

