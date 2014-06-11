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

#ifndef HatoholArmPluginInterfaces_h
#define HatoholArmPluginInterfaces_h

#include <string>
#include <SimpleSemaphore.h>
#include <SmartBuffer.h>
#include <AtomicValue.h>
#include <MutexLock.h>
#include <qpid/messaging/Connection.h>
#include "HatoholArmPluginInterface.h"

class HapiTestHelper {
public:
	HapiTestHelper(void);
	void onConnected(qpid::messaging::Connection &conn);
	void onInitiated(void);

	mlpl::SimpleSemaphore &getConnectedSem(void);
	mlpl::SimpleSemaphore &getInitiatedSem(void);

private:
	mlpl::SimpleSemaphore   m_connectedSem;
	mlpl::SimpleSemaphore   m_initiatedSem;
};

class HatoholArmPluginInterfaceTestBasic :
  public HatoholArmPluginInterface, public HapiTestHelper
{
public:
	static const size_t TIMEOUT = 5000; // ms

	HatoholArmPluginInterfaceTestBasic(
	  const std::string &addr = "test-hatohol-arm-plugin-interface",
	  const bool workInServer = true);

	virtual void onConnected(qpid::messaging::Connection &conn) override;
	virtual void onInitiated(void) override;

	uint32_t callGetIncrementedSequenceId(void);
	void callSetSequenceId(const uint32_t &sequenceId);

	template<class BodyType>
	BodyType *callSetupResponseBuffer(
	  mlpl::SmartBuffer &resBuf, const size_t &additionalSize = 0,
	  const HapiResponseCode &code = HAPI_RES_OK)
	{
		return setupResponseBuffer<BodyType>(resBuf, additionalSize,
		                                     code);
	}

	void assertStartAndWaitConnected(void);
};

class HatoholArmPluginInterfaceTest : public HatoholArmPluginInterfaceTestBasic {
public:
	HatoholArmPluginInterfaceTest(void);

	/**
	 * Create a Hapi instance for client.
	 */
	HatoholArmPluginInterfaceTest(HatoholArmPluginInterfaceTest &hapiSv);

	virtual void onReceived(mlpl::SmartBuffer &smbuf) override;
	mlpl::SimpleSemaphore &getRcvSem(void);

	std::string getMessage(void);
	void setMessageIntercept(void);

protected:
	void setMessage(const mlpl::SmartBuffer &smbuf);

private:
	mlpl::SimpleSemaphore   m_rcvSem;
	mlpl::MutexLock         m_lock;
	std::string             m_message;
	mlpl::AtomicValue<bool> m_msgIntercept;
};

#endif // HatoholArmPluginInterfaces_h
