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

#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
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

struct Params {
	bool dontExit;

	Params(void)
	: dontExit(false)
	{
	}

	void set(const string &key, const string &value)
	{
		if (key == "dontExit") {
			dontExit = atoi(value.c_str());
		} else {
			MLPL_WARN("Unknown paramter: %s (%s)\n",
			          key.c_str(), value.c_str());
		}
	}
};

class TestPlugin : public HatoholArmPluginInterface {
public:
	TestPlugin(void)
	: m_sem(0)
	{
		parseParamFile();

		const char *envQueueAddr =
		  getenv(HatoholArmPluginGate::ENV_NAME_QUEUE_ADDR);
		if (!envQueueAddr) {
			HatoholArmPluginError hapError;
			hapError.code = HAPERR_NOT_FOUND_QUEUE_ADDR;
			onGotError(hapError);
			return;
		}
		setQueueAddress(envQueueAddr);
	}

	void waitSent(void)
	{
		m_sem.wait();
	}

	const Params &getParams(void) const
	{
		return m_params;
	}

protected:
	virtual void onConnected(Connection &conn) override
	{
		send(testMessage);
		m_sem.post();
	}

	void parseParamFile(void)
	{
		string path = StringUtils::sprintf(
		  HAPI_TEST_PLUGIN_PARAM_FILE_FMT, getppid());
		ifstream ifs(path.c_str());
		if (!ifs)
			return;

		string line;
		while (getline(ifs, line)) {
			size_t pos = line.find('\t');
			if (pos == string::npos) {
				MLPL_ERR("Not found separator: %s\n",
				         line.c_str());
				continue;
			}
			string key = string(line, 0, pos);
			string value = string(line, pos+1);
			m_params.set(key, value);
		}

	}

private:
	SimpleSemaphore m_sem;
	Params          m_params;
};

int main(void)
{
	TestPlugin plugin;
	plugin.start();
	plugin.waitSent();

	if (plugin.getParams().dontExit) {
		while (true)
			sleep(3600);
	}

	return EXIT_SUCCESS;
}
