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

#ifndef HapZabbixAPI_h
#define HapZabbixAPI_h

#include <SmartTime.h>
#include "ZabbixAPI.h"
#include "HatoholArmPluginBase.h"

class HapZabbixAPI : public HatoholArmPluginBase, public ZabbixAPI {
public:
	HapZabbixAPI(void);
	virtual ~HapZabbixAPI();

protected:
	void workOnTriggers(void);
	void workOnHostsAndHostgroupElements(void);
	void workOnHostgroups(void);
	void workOnEvents(void);

	virtual void onInitiated(void) override;
	virtual void onReady(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // HapZabbixAPI_h
