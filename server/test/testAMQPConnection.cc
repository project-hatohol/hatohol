/*
 * Copyright (C) 2015 Project Hatohol
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

#include <gcutter.h>
#include <cppcutter.h>

#include <AMQPConnection.h>
#include <AMQPConsumer.h>
#include <AMQPPublisher.h>
#include <AMQPMessageHandler.h>

using namespace std;

namespace testAMQPConnection {
	AMQPConnectionInfo *info;
	AMQPConnectionPtr connection;

	class TestHandler : public AMQPMessageHandler {
		virtual bool handle(const amqp_envelope_t *envelope) override
		{
			return true;
		}
	};

	void cut_setup(void)
	{
		const char *url = getenv("TEST_AMQP_URL");
		//url = "amqp://hatohol:hatohol@localhost:5672/hatohol";
		if (!url)
			cut_omit("TEST_AMQP_URL isn't set");

		info = new AMQPConnectionInfo();
		info->setURL(url);
		info->setQueueName("test.1");

		connection = AMQPConnection::create(*info);
	}
	
	void cut_teardown(void)
	{
		connection->purge();
		connection = NULL;
		delete info;
	}

	void test_connect(void) {
		cppcut_assert_equal(true, connection->connect());
	}

	void test_consumer(void) {
		TestHandler handler;
		AMQPConsumer consumer(connection, &handler);
		consumer.start();
		// TODO: add assertion
		consumer.exitSync();
	}

	void test_publisher(void) {
		AMQPPublisher publisher(connection, "{\"body\":\"example\"}");
		cppcut_assert_equal(true, publisher.publish());
	}
} // namespace testAMQPConnection
