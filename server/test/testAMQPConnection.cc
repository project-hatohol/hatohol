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
#include <atomic>

#include <Reaper.h>
#include <AMQPConnection.h>
#include <AMQPConsumer.h>
#include <AMQPPublisher.h>
#include <AMQPMessageHandler.h>

using namespace std;
using namespace mlpl;

namespace testAMQPConnection {
	AMQPConnectionInfo *connectionInfo;
	shared_ptr<AMQPConnection> connection;

	class TestMessageHandler : public AMQPMessageHandler {
	public:
		TestMessageHandler()
		: m_gotMessage(false)
		{
		}

		virtual bool handle(AMQPConsumer &consumer,
				    const AMQPMessage &message) override
		{
			m_gotMessage = true;
			m_message = message;
			return true;
		}

		atomic<bool> m_gotMessage;
		AMQPMessage m_message;
	};

	AMQPConnectionInfo &getConnectionInfo(void)
	{
		if (!connectionInfo)
			cut_omit("TEST_AMQP_URL isn't set");
		return *connectionInfo;
	}

	shared_ptr<AMQPConnection> getConnection(void)
	{
		return AMQPConnection::create(getConnectionInfo());
	}

	void cut_setup(void)
	{
		// e.g.) url = "amqp://hatohol:hatohol@localhost:5672/hatohol";
		const char *url = getenv("TEST_AMQP_URL");
		if (url) {
			connectionInfo = new AMQPConnectionInfo();
			connectionInfo->setURL(url);
			connectionInfo->setConsumerQueueName("test.1");
			connectionInfo->setPublisherQueueName("test.1");
		} else {
			connectionInfo = NULL;
		}
	}

	void cut_teardown(void)
	{
		if (connection)
			connection->deleteAllQueues();
		connection = NULL;
		delete connectionInfo;
		connectionInfo = NULL;
	}

	GTimer *startTimer(void)
	{
		GTimer *timer = g_timer_new();
		cut_take(timer, (CutDestroyFunction)g_timer_destroy);
		g_timer_start(timer);
		return timer;
	}

	void test_connect(void)
	{
		connection = getConnection();
		cppcut_assert_equal(true, connection->connect());
	}

	void test_consumeWithoutConnection(void)
	{
		AMQPMessage message;
		connection = getConnection();
		cppcut_assert_equal(false, connection->consume(message));
	}

	void test_publishWithoutConnection(void)
	{
		AMQPMessage message;
		message.body = "hoge";
		connection = getConnection();
		cppcut_assert_equal(false, connection->publish(message));
	}

	void test_transferMessage(void)
	{
		AMQPJSONMessage message;
		message.body = "{\"body\":\"example\"}";
		AMQPPublisher publisher(getConnectionInfo());
		publisher.setMessage(message);
		cppcut_assert_equal(true, publisher.publish());

		TestMessageHandler handler;
		AMQPConsumer consumer(*connectionInfo, &handler);
		consumer.start();
		gdouble timeout = 2.0, elapsed = 0.0;
		GTimer *timer = startTimer();
		while (!handler.m_gotMessage && elapsed < timeout) {
			g_usleep(0.1 * G_USEC_PER_SEC);
			elapsed = g_timer_elapsed(timer, NULL);
		}
		consumer.exitSync();

		cut_assert_true(elapsed < timeout);
		cppcut_assert_equal(message.contentType,
				    handler.m_message.contentType);
		cppcut_assert_equal(message.body,
				    handler.m_message.body);
	}
} // namespace testAMQPConnection
