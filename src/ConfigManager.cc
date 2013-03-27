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

#include "ConfigManager.h"

GMutex ConfigManager::m_mutex;
ConfigManager *ConfigManager::m_instance = NULL;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ConfigManager *ConfigManager::getInstance(void)
{
	if (m_instance)
		return m_instance;

	g_mutex_lock(&m_mutex);
	if (!m_instance)
		m_instance = new ConfigManager();
	g_mutex_unlock(&m_mutex);

	return m_instance;
}

void addTargetServer(MonitoringServerInfo *monitoringServerInfo)
{
	// TODO: implement
}

void getTargetServers(MonitoringServerInfoList &monitoringServers)
{
	// TODO: implement
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
ConfigManager::ConfigManager(void)
{
}

ConfigManager::~ConfigManager()
{
}
