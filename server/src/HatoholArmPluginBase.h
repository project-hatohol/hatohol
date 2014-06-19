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
	struct AsyncCbData;

	HatoholArmPluginBase(void);
	virtual ~HatoholArmPluginBase();

	/**
	 * Get the monitoring server information from the server.
	 *
	 * @param serverInfo Obtained data is set to this object.
	 * @return true on success. Or false is returned.
	 */
	bool getMonitoringServerInfo(MonitoringServerInfo &serverInfo);

	class GetMonitoringServerInfoAsyncArg {
	public:
		GetMonitoringServerInfoAsyncArg(
		  MonitoringServerInfo *serverInfo);
		virtual void doneCb(const bool &succeeded);
		MonitoringServerInfo &getMonitoringServerInfo(void);
	private:
		MonitoringServerInfo *m_serverInfo;
	};

	void getMonitoringServerInfoAsync(GetMonitoringServerInfoAsyncArg *arg);

	mlpl::SmartTime getTimestampOfLastTrigger(void);

	/**
	 * Get the last event ID in the Hatohol server.
	 *
	 * @return
	 * A last event ID. If the server dosen't have events,
	 * EVENT_NOT_FOUND is returned.
	 */
	EventIdType getLastEventId(void);

protected:
	virtual void onGotResponse(const HapiResponseHeader *header,
	                           mlpl::SmartBuffer &resBuf) override;

	void sendCmdGetMonitoringServerInfo(void);
	bool getMonitoringServerInfoBottomHalf(
	  MonitoringServerInfo &serverInfo);
	static void _getMonitoringServerInfoAsyncCb(AsyncCbData *data);
	void getMonitoringServerInfoAsyncCb(GetMonitoringServerInfoAsyncArg *);

	void waitResponseAndCheckHeader(void);
	void sendTable(const HapiCommandCode &code,
	               const ItemTablePtr &tablePtr);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // HatoholArmPluginBase_h
