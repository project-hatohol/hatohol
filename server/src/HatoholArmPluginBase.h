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
#include "ArmStatus.h"
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
	static const size_t WAIT_INFINITE;

	virtual void onGotResponse(const HapiResponseHeader *header,
	                           mlpl::SmartBuffer &resBuf) override;

	virtual void onInitiated(void) override;

	/**
	 * Called when the terminate command is received. The default
	 * behavior is calling exit(EXIT_SUCCESS).
	 */
	virtual void onReceivedTerminate(void);
	virtual void onPreWaitInitiatedAck(void);
	virtual void onPostWaitInitiatedAck(void);

	void sendCmdGetMonitoringServerInfo(CommandCallbacks *callbacks);
	bool parseReplyGetMonitoringServerInfo(
	  MonitoringServerInfo &serverInfo,
	  const mlpl::SmartBuffer &responseBuf);
	static void _getMonitoringServerInfoAsyncCb(AsyncCbData *data);
	void getMonitoringServerInfoAsyncCb(GetMonitoringServerInfoAsyncArg *);

	void waitResponseAndCheckHeader(void);

	/**
	 * Enable/disable the wait for an acknowlege in onInitiated().
	 * After this method is called with enable=true, onIniated() is blocked
	 * until ackInitiated() is called.
	 *
	 * @param enable A flag to enable or disable.
	 */
	void enableWaitInitiatedAck(const bool &enable = true);
	void ackInitiated(void);

	/**
	 * Throw InitiatedException if this instance waits for
	 * an acknowlege in onInitiated().
	 */
	void throwInitiatedExceptionIfNeeded(void);

	/**
	 * Sleep until wake() is called.
	 *
	 * @param timeoutInMSec
	 * A timeout value in millisecond. If WAIT_INFINITE is specified,
	 * it isn't timed out.
	 *
	 * @return
	 * true if wake() is called within the timeout. Otherwise false.
	 */
	bool sleepInitiatedExceptionThrowable(size_t timeoutInMSec);
	void wake(void);

	void sendTable(const HapiCommandCode &code,
	               const ItemTablePtr &tablePtr);
	void sendArmInfo(const ArmInfo &armInfo);

	void cmdHandlerTerminate(const HapiCommandHeader *header);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // HatoholArmPluginBase_h
