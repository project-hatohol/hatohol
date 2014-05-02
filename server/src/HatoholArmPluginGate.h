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
#include "HatoholThreadBase.h"
#include "DataStore.h"
#include "UsedCountablePtr.h"

class HatoholArmPluginGate : public DataStore, public HatoholThreadBase {
public:
	static const std::string PassivePluginQuasiPath;
	static const char *DEFAULT_BROKER_URL;
	static const char *ENV_NAME_QUEUE_ADDR;
	static const int   NO_RETRY;

	HatoholArmPluginGate(const MonitoringServerInfo &serverInfo);

	/**
	 * Start an initiation. This typically launch a plugin process.
	 *
	 * @param type A monitoring system type.
	 *
	 * @return true if successfully started. Or false is returned.
	 */
	bool start(const MonitoringSystemType &type);

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
	virtual ArmBase &getArmBase(void) // override
	{
		return *((ArmBase *)NULL);
	}

	// virtual methods

	/**
	 * Wait for a complete exit of the thread.
	 */
	virtual void waitExit(void); // override

	gpointer mainThread(HatoholThreadArg *arg); // override

protected:
	// To avoid an instance from being created on a stack.
	virtual ~HatoholArmPluginGate();

	virtual void onSessionChanged(qpid::messaging::Session *session);
	virtual void onReceived(qpid::messaging::Message &message);
	virtual void onTerminated(const siginfo_t *siginfo);

	/**
	 * Called when an exception was caught.
	 *
	 * @param e An exception instance.
	 *
	 * @return
	 * A sleep time until the next retry to connect with Qpidd in
	 * millisecond. If the value is NO_RETRY, the retry won't be done.
	 */
	virtual int onCaughtException(const std::exception &e);

	bool launchPluginProcess(const ArmPluginInfo &armPluginInfo);
	static std::string generateBrokerAddress(
	  const MonitoringServerInfo &serverInfo);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;;
};

typedef UsedCountablePtr<HatoholArmPluginGate> HatoholArmPluginGatePtr;

#endif // HatoholArmPluginGate_h

