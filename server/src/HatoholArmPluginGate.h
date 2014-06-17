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
	static const char *ENV_NAME_QUEUE_ADDR;
	static const int   NO_RETRY;

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
	virtual ArmBase &getArmBase(void) override
	{
		return *((ArmBase *)NULL);
	}

	// virtual methods

	/**
	 * Wait for a complete exit of the thread.
	 */
	virtual void exitSync(void) override;

protected:
	// To avoid an instance from being created on a stack.
	virtual ~HatoholArmPluginGate();

	virtual void onConnected(qpid::messaging::Connection &conn) override;

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

	bool launchPluginProcess(const ArmPluginInfo &armPluginInfo);
	static std::string generateBrokerAddress(
	  const MonitoringServerInfo &serverInfo);

	void cmdHandlerGetMonitoringServerInfo(
	  const HapiCommandHeader *header);
	void cmdHandlerGetTimestampOfLastTrigger(
	  const HapiCommandHeader *header);
	void cmdHandlerSendUpdatedTriggers(const HapiCommandHeader *header);
	void cmdHandlerSendHosts(const HapiCommandHeader *header);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;;
};

typedef UsedCountablePtr<HatoholArmPluginGate> HatoholArmPluginGatePtr;

#endif // HatoholArmPluginGate_h

