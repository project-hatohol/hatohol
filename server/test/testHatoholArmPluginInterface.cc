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

#include <cppcutter.h>
#include "Helpers.h"
#include "HatoholArmPluginInterface.h"

using namespace std;
using namespace mlpl;

namespace testHatoholArmPluginInterface {

struct TestContext {
	AtomicValue<bool> timedout;
	AtomicValue<bool> connected;

	TestContext(void)
	: timedout(false),
	  connected(false)
	{
	}
};

class TestHatoholArmPluginInterface : public HatoholArmPluginInterface {
public:
	TestHatoholArmPluginInterface(
	  const string &addr = "test-hatohol-arm-plugin-interface; {create: always}")
	: HatoholArmPluginInterface(addr)
	{
	}

	virtual void onConnected(void) // override
	{
		m_testCtx.connected = true;
		m_loop.quit();
	}

	void run(void)
	{
		start();
		m_loop.run(timeoutCb, this);
	}

	TestContext &getTestContext(void)
	{
		return m_testCtx;
	}

protected:
	static gboolean timeoutCb(gpointer data)
	{
		TestHatoholArmPluginInterface *obj =
		  static_cast<TestHatoholArmPluginInterface *>(data);
		obj->m_testCtx.timedout = true;
		obj->m_loop.quit();
		return G_SOURCE_REMOVE;
	}

private:
	GMainLoopAgent m_loop;
	TestContext    m_testCtx;
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	HatoholArmPluginInterface hapi;
}

void test_onConnected(void)
{
	TestHatoholArmPluginInterface hapi;
	hapi.run();
	cppcut_assert_equal(false, (bool)hapi.getTestContext().timedout);
	cppcut_assert_equal(true, (bool)hapi.getTestContext().connected);
}

} // namespace testHatoholArmPluginInterface
