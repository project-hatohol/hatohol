/*
 * Copyright (C) 2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "ArmPluginInfo.h"

void ArmPluginInfo::initialize(ArmPluginInfo &armPluginInfo)
{
	armPluginInfo.id = INVALID_ARM_PLUGIN_INFO_ID;
	armPluginInfo.type = MONITORING_SYSTEM_UNKNOWN;
	armPluginInfo.serverId = INVALID_SERVER_ID;
	armPluginInfo.tlsEnableVerify = 1;
}

bool ArmPluginInfo::isTLSVerifyEnabled(void) const
{
	return tlsEnableVerify != 0;
}
