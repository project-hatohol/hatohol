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

enum HapiCommandCode {
	HAPI_CMD_GET_LAST_TRIGGER_TIME,
	NUM_HAPI_CMD
};

enum HapiResponseCode {
	HAPI_RES_OK,
	HAPI_RES_INVALID_HEADER,
	HAPI_RES_UNKNOWN_CODE,
	HAPI_RES_INVALID_ARG,
	NUM_HAPI_CMD_RES
};

struct HapiCommandHeader {
	uint16_t code;
};

struct HapiResponseHeader {
	uint16_t code;
};

class HatoholArmPluginInterface : public HatoholThreadBase {
public:
	typedef void (HatoholArmPluginInterface::*CommandHandler)(
	  const HapiCommandHeader *header);

	HatoholArmPluginInterface(const std::string &queueAddr = "");
	virtual ~HatoholArmPluginInterface() override;

	virtual void exitSync(void) override;

	void setQueueAddress(const std::string &queueAddr);
	void send(const std::string &message);
	void send(const mlpl::SmartBuffer &smbuf);

	void reply(const mlpl::SmartBuffer &replyBuf);
	void replyError(const HapiResponseCode &code);

	/**
	 * Register a message receive callback method.
	 * If the same code is specified more than twice, the handler is
	 * updated.
	 *
	 * @param code HapiCommandCode or HapiServiceCode.
	 * @param handler A receive handler.
	 */
	void registCommandHandler(const HapiCommandCode &code,
	                          CommandHandler handler);

protected:
	typedef std::map<uint16_t, CommandHandler> CommandHandlerMap;
	typedef CommandHandlerMap::iterator        CommandHandlerMapIterator;
	typedef CommandHandlerMap::const_iterator  CommandHandlerMapConstIterator;

	virtual gpointer mainThread(HatoholThreadArg *arg) override;

	/**
	 * Called when connection with the AMQP broker is established.
	 *
	 * @param conn A connection object
	 */
	virtual void onConnected(qpid::messaging::Connection &conn);

	/**
	 * Called when the session is changed.
	 *
	 * @param session
	 * An session instance or NULL if the session is lost.
	 */
	virtual void onSessionChanged(qpid::messaging::Session *session);

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
