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

#include <Reaper.h>
#include <AtomicValue.h>
#include <AMQPConnection.h>
#include <AMQPConsumer.h>
#include <AMQPPublisher.h>
#include <AMQPMessageHandler.h>

using namespace std;
using namespace mlpl;

namespace testAMQPConnection {
	AMQPConnectionInfo *info;
	AMQPConnectionPtr connection;

	class TestHandler : public AMQPMessageHandler {
	public:
		TestHandler()
		: m_gotMessage(false)
		{
		}

		virtual bool handle(const amqp_envelope_t *envelope) override
		{
			const amqp_bytes_t *contentType =
			  &(envelope->message.properties.content_type);
			const amqp_bytes_t *body = &(envelope->message.body);
			m_contentType.assign(static_cast<char*>(contentType->bytes),
					     static_cast<int>(contentType->len));
			m_body.assign(static_cast<char*>(body->bytes),
				      static_cast<int>(body->len));
			m_gotMessage = true;
			return true;
		}

		AtomicValue<bool> m_gotMessage;
		string m_contentType;
		string m_body;
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
		string body = "{\"body\":\"example\"}";
		connection->connect();
		connection->publish(body);

		TestHandler handler;
		AMQPConsumer consumer(connection, &handler);

		consumer.start();
		while (!handler.m_gotMessage)
			g_usleep(0.1 * G_USEC_PER_SEC);
		consumer.exitSync();

		cppcut_assert_equal(string("application/json"),
				    handler.m_contentType);
		cppcut_assert_equal(body, handler.m_body);
	}

	void test_publisher(void) {
		AMQPPublisher publisher(connection, "{\"body\":\"example\"}");
		cppcut_assert_equal(true, publisher.publish());
	}
} // namespace testAMQPConnection
