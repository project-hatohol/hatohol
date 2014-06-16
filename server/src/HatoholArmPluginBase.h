/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HatoholArmPluginBase_h
#define HatoholArmPluginBase_h

#include <SmartTime.h>
#include "ItemTablePtr.h"
#include "MonitoringServerInfo.h"
#include "HatoholArmPluginInterface.h"

class HatoholArmPluginBase : public HatoholArmPluginInterface {
public:
	HatoholArmPluginBase(void);
	virtual ~HatoholArmPluginBase();

	/**
	 * Get the monitoring server information from the server.
	 *
	 * @param serverInfo Obtained data is set to this object.
	 * @return true on success. Or false is returned.
	 */
	bool getMonitoringServerInfo(MonitoringServerInfo &serverInfo);

	mlpl::SmartTime getTimestampOfLastTrigger(void);

protected:
	virtual void onGotResponse(const HapiResponseHeader *header,
	                           mlpl::SmartBuffer &resBuf) override;

	void waitResponseAndCheckHeader(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // HatoholArmPluginBase_h
