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
#include <unistd.h>
#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include "HatoholArmPluginGate.h"
#include "hapi-test-plugin.h"

using namespace std;
using namespace qpid::messaging;

int main(void)
{
	const char *envQueueAddr =
	  getenv(HatoholArmPluginGate::ENV_NAME_QUEUE_ADDR);
	if (!envQueueAddr) {
		printf("Not found environment variable: %s\n",
		       HatoholArmPluginGate::ENV_NAME_QUEUE_ADDR);
		abort();
	}
	const string queueAddr = envQueueAddr;

	const string brokerUrl = HatoholArmPluginGate::DEFAULT_BROKER_URL;
	const string connectionOptions;
	Connection connection(brokerUrl, connectionOptions);
	try {
		connection.open();
		Session session = connection.createSession();
		Sender sender = session.createSender(queueAddr);
		Message request;
		request.setContent(testMessage);
		sender.send(request);
	} catch (const exception &error) {
		printf("Got exception: %s\n", error.what());
		abort();
	}
	connection.close();
	sleep(3600);

	return EXIT_SUCCESS;
}
