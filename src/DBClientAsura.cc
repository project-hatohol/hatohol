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

#include <memory>

#include "DBAgentFactory.h"
#include "DBClientAsura.h"

struct DBClientAsura::PrivateContext
{
	static GMutex mutex;

	static void lock(void)
	{
		g_mutex_lock(&mutex);
	}

	static void unlock(void)
	{
		g_mutex_unlock(&mutex);
	}
};
GMutex DBClientAsura::PrivateContext::mutex;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBClientAsura::DBClientAsura(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClientAsura::~DBClientAsura()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
