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

#include "ReadWriteLock.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ReadWriteLock::ReadWriteLock(void)
{
#ifdef GLIB_VERSION_2_32
	g_rw_lock_init(&m_lock);
#else
	g_static_mutex_init(&m_lock);
#endif // GLIB_VERSION_2_32
}

ReadWriteLock::~ReadWriteLock()
{
#ifdef GLIB_VERSION_2_32
	g_rw_lock_clear(&m_lock);
#else
	g_static_mutex_free(&m_lock);
#endif // GLIB_VERSION_2_32
}

void ReadWriteLock::readLock(void) const
{
#ifdef GLIB_VERSION_2_32
	g_rw_lock_reader_lock(&m_lock);
#else
	g_static_mutex_lock(&m_lock);
#endif // GLIB_VERSION_2_32
}

void ReadWriteLock::readUnlock(void) const
{
#ifdef GLIB_VERSION_2_32
	g_rw_lock_reader_unlock(&m_lock);
#else
	g_static_mutex_unlock(&m_lock);
#endif // GLIB_VERSION_2_32
}

void ReadWriteLock::writeLock(void) const
{
#ifdef GLIB_VERSION_2_32
	g_rw_lock_writer_lock(&m_lock);
#else
	g_static_mutex_lock(&m_lock);
#endif // GLIB_VERSION_2_32
}

void ReadWriteLock::writeUnlock(void) const
{
#ifdef GLIB_VERSION_2_32
	g_rw_lock_writer_unlock(&m_lock);
#else
	g_static_mutex_unlock(&m_lock);
#endif // GLIB_VERSION_2_32
}

