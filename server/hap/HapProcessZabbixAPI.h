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

#include <exception>
#include <HapZabbixAPI.h>
#include "HapProcess.h"

class HapProcessZabbixAPI : public HapProcess, public HapZabbixAPI {
public:
	HapProcessZabbixAPI(int argc, char *argv[]);
	virtual ~HapProcessZabbixAPI();
	int mainLoopRun(void);
	virtual int onCaughtException(const std::exception &e) override;

protected:
	// called from HapZabbixAPI
	void onReady(const MonitoringServerInfo &serverInfo) override;

	static gboolean acquisitionTimerCb(void *data);
	void startAcquisition(void);
	void acquireData(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;;
};

