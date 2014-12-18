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

#ifndef HatoholArmPluginBase_h
#define HatoholArmPluginBase_h

#include <SmartTime.h>
#include "ArmStatus.h"
#include "ItemTablePtr.h"
#include "MonitoringServerInfo.h"
#include "HatoholArmPluginInterface.h"

class HatoholArmPluginBase : public HatoholArmPluginInterface {
public:
	class SyncCommand;

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

	/**
	 * Get the last event ID in the Hatohol server.
	 *
	 * @return
	 * A last event ID. If the server dosen't have events,
	 * EVENT_NOT_FOUND is returned.
	 */
	EventIdType getLastEventId(void);

	/**
	 * Get the time of the last event.
	 *
	 * @param triggerId An ID of the tareget trigger.
	 *
	 * @return
	 * The time of the last event is returned. If no events found,
	 * hasValidTime() of the retruned value is false.
	 */
	mlpl::SmartTime getTimeOfLastEvent(
	  const TriggerIdType &triggerId = ALL_TRIGGERS);

protected:
	static const size_t WAIT_INFINITE;

	/**
	 * Called when the terminate command is received. The default
	 * behavior is calling exit(EXIT_SUCCESS).
	 */
	virtual void onReceivedTerminate(void);
	virtual void onFailureReceivedMessage(void) override;

	void sendCmdGetMonitoringServerInfo(CommandCallbacks *callbacks);
	bool parseReplyGetMonitoringServerInfo(
	  MonitoringServerInfo &serverInfo,
	  const mlpl::SmartBuffer &responseBuf);

	void sendTable(const HapiCommandCode &code,
	               const ItemTablePtr &tablePtr);
	void sendArmInfo(const ArmInfo &armInfo,
			 const HatoholArmPluginWatchType &type = COLLECT_OK);
	void sendAvailableTrigger(const int TriggerNum,
				  const HatoholArmPluginWatchType *TriggerList);
	void cmdHandlerTerminate(const HapiCommandHeader *header);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // HatoholArmPluginBase_h
