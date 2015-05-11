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

using namespace std;

namespace testAMQPConnection {
	AMQPConnectionInfo *info;

	void cut_setup(void)
	{
		const char *url = getenv("TEST_AMQP_URL");
		//url = "amqp://hatohol:hatohol@localhost:5672/hatohol";
		if (!url)
			cut_omit("TEST_AMQP_URL isn't set");

		info = new AMQPConnectionInfo();
		info->setURL(url);
		info->setQueueName("test.1");
	}
	
	void cut_teardown(void)
	{
		delete info;
	}

	void test_connect(void) {
		AMQPConnection connection(*info);
		cppcut_assert_equal(true, connection.connect());
	}
} // namespace testAMQPConnection
