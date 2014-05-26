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

#include <cstdio>
#include <cstdlib>
#include <string>
#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include <SimpleSemaphore.h>
#include "HatoholArmPluginGate.h" // TODO: remove after ENV_NAME_QUEUE_ADDR is moved to HatoholArmPluginInterface.h 
#include "HatoholArmPluginInterface.h"
#include "hapi-test-plugin.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

class TestPlugin : public HatoholArmPluginInterface {
public:
	TestPlugin(void)
	: m_sem(0)
	{
		const char *envQueueAddr =
		  getenv(HatoholArmPluginGate::ENV_NAME_QUEUE_ADDR);
		if (!envQueueAddr) {
			HatoholArmPluginError hapError;
			hapError.code = HAPERR_NOT_FOUND_QUEUE_ADDR;
			onGotError(hapError);
			return;
		}
		string queueAddr = envQueueAddr;
		queueAddr += "; {create: always}";
		setQueueAddress(queueAddr);
	}

	void waitSent(void)
	{
		m_sem.wait();
	}

protected:
	virtual void onConnected(Connection &conn) // override
	{
		send(testMessage);
		m_sem.post();
	}

private:
	SimpleSemaphore m_sem;
};

int main(void)
{
	TestPlugin plugin;
	plugin.start();
	plugin.waitSent();
	return EXIT_SUCCESS;
}
