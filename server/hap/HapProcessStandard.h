/*
 * Copyright (C) 2014 Project Hatohol
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

#ifndef HapProcessStandard_h
#define HapProcessStandard_h

#include "HapProcess.h"
#include "HatoholArmPluginStandard.h"

class HapProcessStandard : public HapProcess, public HatoholArmPluginStandard {
public:
	HapProcessStandard(int argc, char *argv[]);
	virtual ~HapProcessStandard();
	int mainLoopRun(void);

protected:
	static gboolean acquisitionTimerCb(void *data);
	void startAcquisition(void);
	const MonitoringServerInfo &getMonitoringServerInfo(void) const;

	virtual HatoholError acquireData(void);

	virtual void onReady(const MonitoringServerInfo &serverInfo) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif /// HapProcessStandard_h

