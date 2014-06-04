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
#include "HatoholException.h"
#include "Utils.h"

enum HatoholArmPluginErrorCode {
	HAPERR_OK,
	HAPERR_NOT_FOUND_QUEUE_ADDR,
};

struct HatoholArmPluginError {
	HatoholArmPluginErrorCode code;
};

enum HapiMessageType {
	HAPI_MSG_COMMAND,
	HAPI_MSG_RESPONSE,
	NUM_HAPI_MSG
};

enum HapiCommandCode {
	HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER,
	NUM_HAPI_CMD
};

enum HapiResponseCode {
	HAPI_RES_OK,
	HAPI_RES_INVALID_HEADER,
	HAPI_RES_UNKNOWN_CODE,
	HAPI_RES_INVALID_ARG,
	NUM_HAPI_CMD_RES
};

// ---------------------------------------------------------------------------
// Structure of the message header
//
// [ 2B] HapiMessageType
//       HapiCommandHeader or HapiResponseHeader
//       Body
// ---------------------------------------------------------------------------
struct HapiCommandHeader {
	uint16_t type;
	uint16_t code;
} __attribute__((__packed__));

struct HapiResponseHeader {
	uint16_t type;
	uint16_t code;
} __attribute__((__packed__));

struct HapiResTimestampOfLastTrigger {
	uint64_t timestamp; // Unix time (GMT)
	uint32_t nanosec;
} __attribute__((__packed__));

class HatoholArmPluginInterface : public HatoholThreadBase {
public:
	typedef void (HatoholArmPluginInterface::*CommandHandler)(
	  const HapiCommandHeader *header);

	HatoholArmPluginInterface(const std::string &queueAddr = "",
	                          const bool &workInServer = false);
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

	const std::string &getQueueAddress(void) const;

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
	 * Called when a HAPI's response is received.
	 *
	 * @param smbuf
	 * A received response. The index of smbuf has been reset to zero.
	 */
	virtual void onGotResponse(const HapiResponseHeader *header,
	                           mlpl::SmartBuffer &resBuf);

	/**
	 * Fill data to a smart buffer.
	 *
	 * @param sbuf    A smart buffer to be setup.
	 * @param message A source message.
	 */
	void load(mlpl::SmartBuffer &sbuf,
	          const qpid::messaging::Message &message);

	void parseCommand(const HapiCommandHeader *header,
	                  mlpl::SmartBuffer &cmdBuf);
	void parseResponse(const HapiResponseHeader *header,
	                   mlpl::SmartBuffer &resBuf);

	/**
	 * Get the response header.
	 *
	 * If the response size is smaller than the size of the header or
	 * code is not HAPI_RES_OK, HatoholException is thrown.
	 *
	 * @param resBuf A response buffer.
	 */
	const HapiResponseHeader *getResponseHeader(mlpl::SmartBuffer &resBuf)
	  throw(HatoholException);

	/**
	 * Get the response body.
	 *
	 * If the response size is smaller than the expected size,
	 * HatoholException is thrown.
	 *
	 * @param resBuf A response buffer.
	 */
	template<class T>
	const T *getResponseBody(mlpl::SmartBuffer &resBuf)
	  throw(HatoholException)
	{
		size_t expectedSize = sizeof(HapiResponseHeader) + sizeof(T);
		if (resBuf.size() < expectedSize) {
			THROW_HATOHOL_EXCEPTION(
			  "Bad size: expected: %zd, actual: %zd (%s)",
			  expectedSize, resBuf.size(), DEMANGLED_TYPE_NAME(T));
		}
		return resBuf.getPointer<T>(sizeof(HapiResponseHeader));
	}

	/**
	 * Allocate buffer to include the specified body and set up
	 * parameters of the header.
	 *
	 * @param resBuf A SmartBuffer instance to be set up.
	 * @return A pointer of the body area.
	 */
	template<class T>
	T *setupResponseBuffer(mlpl::SmartBuffer &resBuf)
	{
		const size_t requiredSize =
		  sizeof(HapiResponseHeader) + sizeof(T);
		resBuf.alloc(requiredSize);
		HapiResponseHeader *header =
		  resBuf.getPointer<HapiResponseHeader>(0);
		header->type = HAPI_MSG_RESPONSE;
		header->code = HAPI_RES_OK;
		return resBuf.getPointer<T>(sizeof(T));
	}

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // HatoholArmPluginInterface_h
