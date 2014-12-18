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

#ifndef HatoholArmPluginGate_h
#define HatoholArmPluginGate_h

#include <qpid/messaging/Message.h>
#include <qpid/messaging/Session.h>
#include "HatoholArmPluginInterface.h"
#include "DataStore.h"
#include "UsedCountablePtr.h"

class HatoholArmPluginGate : public DataStore, public HatoholArmPluginInterface {
public:
	static const std::string PassivePluginQuasiPath;
	static const int   NO_RETRY;
	
	struct HAPIWtchPointInfo {
		TriggerStatusType statusType;
		TriggerIdType triggerId;
		std::string msg;
	};

	HatoholArmPluginGate(const MonitoringServerInfo &serverInfo);

	/**
	 * Start the plugin gate thread.
	 */
	void start(void);

	/**
	 * Reutrn an ArmStatus instance.
	 *
	 * Note that the caller don't make a copy of the returned value on a
	 * stack. It causes the double free.
	 *
	 * @return an ArmStatusInstance.
	 */
	const ArmStatus &getArmStatus(void) const;

	// This is dummy and this virtual method should be removed
	virtual ArmBase &getArmBase(void) override;

	// virtual methods

	/**
	 * Wait for a complete exit of the thread.
	 */
	virtual void exitSync(void) override;

	/**
	 * Get the PID of the plugin process.
	 *
	 * @return
	 * The PID of the plugin process. If the plugin process dones't
	 * exist including the passve mode, 0 is returned.
	 */
	pid_t getPid(void);

protected:
	// To avoid an instance from being created on a stack.
	virtual ~HatoholArmPluginGate();

	virtual void onSetPluginInitialInfo(void) override;

	virtual void onConnected(qpid::messaging::Connection &conn) override;
	virtual void onInitiated(void) override;
	/**
	 * Called when an exception was caught.
	 *
	 * @param e An exception instance.
	 *
	 * @return
	 * A sleep time until the next retry to connect with Qpidd in
	 * millisecond. If the value is NO_RETRY, the retry won't be done.
	 */
	virtual int onCaughtException(const std::exception &e) override;

	virtual void onLaunchedProcess(
	  const bool &succeeded, const ArmPluginInfo &armPluginInfo);
	virtual void onTerminated(const siginfo_t *siginfo);

	virtual void onFailureConnected(void) override;
	virtual void onPriorToFetchMessage(void) override;
	virtual void onSuccessFetchMessage(void) override;
	virtual void onFailureFetchMessage(void) override;
	virtual void onFailureReceivedMessage(void) override;

	/**
	 * Terminates the plugin and wait for it.
	 */
	virtual void terminatePluginSync(void);

	bool launchPluginProcess(const ArmPluginInfo &armPluginInfo);
	static std::string generateBrokerAddress(
	  const MonitoringServerInfo &serverInfo);
	void sendTerminateCommand(void);

	void cmdHandlerGetMonitoringServerInfo(
	  const HapiCommandHeader *header);
	void cmdHandlerGetTimestampOfLastTrigger(
	  const HapiCommandHeader *header);
	void cmdHandlerGetLastEventId(const HapiCommandHeader *header);
	void cmdHandlerGetTimeOfLastEvent(const HapiCommandHeader *header);
	void cmdHandlerSendUpdatedTriggers(const HapiCommandHeader *header);
	void cmdHandlerSendHosts(const HapiCommandHeader *header);
	void cmdHandlerSendHostgroupElements(const HapiCommandHeader *header);
	void cmdHandlerSendHostgroups(const HapiCommandHeader *header);
	void cmdHandlerSendUpdatedEvents(const HapiCommandHeader *header);
	void cmdHandlerSendArmInfo(const HapiCommandHeader *header);
	void cmdHandlerAvailableTrigger(const HapiCommandHeader *header);

	void addInitialTrigger(HatoholArmPluginWatchType addtrigger);

	void createPluginTriggerInfo(const HAPIWtchPointInfo &resTrigger,
				     TriggerInfoList &triggerInfoList);
	void createPluginEventInfo(const HAPIWtchPointInfo &resTrigger,
				   EventInfoList &eventInfoList);
	void setPluginConnectStatus(const HatoholArmPluginWatchType &type,
				    const HatoholArmPluginErrorCode &errorCode);
	void setPluginAvailabelTrigger(const HatoholArmPluginWatchType &type,
				       const TriggerIdType &trrigerId,
				       const HatoholError &hatoholError);

	static gboolean detectedArmPluginTimeout(void *data);
	static void removeArmPluginTimeout(gpointer data);
private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;;
};

typedef UsedCountablePtr<HatoholArmPluginGate> HatoholArmPluginGatePtr;

#endif // HatoholArmPluginGate_h

