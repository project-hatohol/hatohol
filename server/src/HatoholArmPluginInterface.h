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

#ifndef HatoholArmPluginInterface_h
#define HatoholArmPluginInterface_h

#include <string>
#include <SmartBuffer.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Connection.h>
#include "HatoholThreadBase.h"

enum HatoholArmPluginErrorCode {
	HAPERR_OK,
	HAPERR_NOT_FOUND_QUEUE_ADDR,
};

struct HatoholArmPluginError {
	HatoholArmPluginErrorCode code;
};

class HatoholArmPluginInterface : public HatoholThreadBase {
public:
	HatoholArmPluginInterface(const std::string &queueAddr = "");
	virtual ~HatoholArmPluginInterface(); // override

	virtual void exitSync(void); // override

	void setQueueAddress(const std::string &queueAddr);
	void send(const std::string &message);

protected:
	virtual gpointer mainThread(HatoholThreadArg *arg); // override

	/**
	 * Called when connection with the AMQP broker is established.
	 *
	 * @param conn A connection object
	 */
	virtual void onConnected(qpid::messaging::Connection &conn);

	/**
	 * Called when a message is received.
	 *
	 * @param smbuf
	 * A received message. The index of smbuf has been reset to zero.
	 */
	virtual void onReceived(mlpl::SmartBuffer &smbuf);
	virtual void onGotError(const HatoholArmPluginError &hapError);

	/**
	 * Fill data to a smart buffer.
	 *
	 * @param sbuf    A smart buffer to be setup.
	 * @param message A source message.
	 */
	void load(mlpl::SmartBuffer &sbuf,
	          const qpid::messaging::Message &message);
private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // HatoholArmPluginInterface_h
