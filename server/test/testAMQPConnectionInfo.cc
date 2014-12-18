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

#include <gcutter.h>
#include <cppcutter.h>

#include <AMQPConnectionInfo.h>

using namespace std;

namespace testAMQPConnectionInfo {
namespace defaultValue {
	AMQPConnectionInfo *info;

	void cut_setup(void)
	{
		info = new AMQPConnectionInfo();
	}
	
	void cut_teardown(void)
	{
		delete info;
	}

	void test_URL(void)
	{
		cppcut_assert_equal(string("amqp://localhost"), info->getURL());
	}

	void test_host(void)
	{
		cppcut_assert_equal("localhost", info->getHost());
	}

	void test_port(void)
	{
		cppcut_assert_equal(5672, info->getPort());
	}

	void test_user(void)
	{
		cppcut_assert_equal("guest", info->getUser());
	}

	void test_password(void)
	{
		cppcut_assert_equal("guest", info->getPassword());
	}

	void test_virtualHost(void)
	{
		cppcut_assert_equal("/", info->getVirtualHost());
	}

	void test_useTLS(void)
	{
		cppcut_assert_equal(false, info->useTLS());
	}

	void test_timeout(void)
	{
		time_t defaultTimeout = 1;
		cppcut_assert_equal(defaultTimeout, info->getTimeout());
	}
}
} // namespace testAMQPConnectionInfo
