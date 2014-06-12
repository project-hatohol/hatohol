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
#include <Serializer.h>
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
	HAPI_MSG_INITIATION,          // Sv -> Cl
	HAPI_MSG_INITIATION_RESPONSE, // Cl -> Sv
	HAPI_MSG_INITIATION_FINISH,   // Sv -> Cl (confirmation)
	HAPI_MSG_COMMAND,
	HAPI_MSG_RESPONSE,
	NUM_HAPI_MSG
};

enum HapiCommandCode {
	HAPI_CMD_GET_MONITORING_SERVER_INFO,
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
struct HapiInitiationPacket {
	uint16_t type;
	uint64_t key;
} __attribute__((__packed__));

struct HapiCommandHeader {
	uint16_t type;
	uint16_t code;
	uint32_t sequenceId;
} __attribute__((__packed__));

struct HapiResponseHeader {
	uint16_t type;
	uint16_t code;
	uint32_t sequenceId;
} __attribute__((__packed__));

struct HapiResMonitoringServerInfo {
	uint32_t serverId;
	uint32_t type;
	uint16_t hostNameLength;  // Not include the NULL terminator
	uint16_t hostNameOffset;  // from the top of this structure
	uint16_t ipAddressLength; // Not include the NULL terminator
	uint16_t ipAddressOffset; // from the top of this structure
	uint16_t nicknameLength;  // Not include the NULL terminator
	uint16_t nicknameOffset;  // from the top of this structure
	uint32_t port;
	uint32_t pollingIntervalSec;
	uint32_t retryIntervalSec;
	// Body of hostName  (including NULL terminator)
	// Body of ipAddress (including NULL terminator)
	// Body of nickname  (including NULL terminator)
} __attribute__((__packed__));

struct HapiResTimestampOfLastTrigger {
	uint64_t timestamp; // Unix time (GMT)
	uint32_t nanosec;
} __attribute__((__packed__));

class HatoholArmPluginInterface : public HatoholThreadBase, public Serializer {
public:
	static const char *DEFAULT_BROKER_URL;
	static const uint32_t SEQ_ID_UNKNOWN;
	static const uint32_t SEQ_ID_MAX;

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
	void registerCommandHandler(const HapiCommandCode &code,
	                            CommandHandler handler);

	const std::string &getQueueAddress(void) const;

	/**
	 * Get a C-style string from the received packet.
	 * The string shall have a NULL terminator.
	 *
	 * @param repBuf An received buffer.
	 * @param head
	 * The reference position (Typically the top of the body).
	 *
	 * @param offset
	 * An offset to the top of the string from the reference point.
	 *
	 * @param length
	 * A length of the string. The NULL terminator is not counted.
	 *
	 * @return
	 * A pointer of the string. If the error happens, NULL is returned.
	 *
	 */
	static const char *getString(
	  const mlpl::SmartBuffer &repBuf,
	  const void *head, const size_t &offset, const size_t &length);

	/**
	 * Write a C-style string to the buffer for the packet in conjunction
	 * with some meta information such as length and offset.
	 *
	 * @param buf
	 * An address where the string is written.
	 *
	 * @param refAddr
	 * A reference address that is used to calculate the offset.
	 *
	 * @param src
	 * A string to be written.
	 *
	 * @param offsetField
	 * An address where the offset (buf - refAddress) is written
	 * as little endian.
	 *
	 * @param lengthField
	 * An address where the length (not including NULL term) is written
	 * as little endian.
	 *
	 * @return
	 * The address next to the written string.
	 */
	static char *putString(
	  void *buf, const void *refAddr, const std::string &src,
	  uint16_t *offsetField, uint16_t *lengthField);

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
	 * Called when initiation with the other side is completed.
	 */
	virtual void onInitiated(void);

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

	void sendInitiationPacket(void);
	void initiation(const mlpl::SmartBuffer &sbuf);
	void waitInitiationResponse(const HapiInitiationPacket *initPkt);
	void replyInitiationPacket(const HapiInitiationPacket *initPkt);
	void finishInitiation(const HapiInitiationPacket *initPkt);

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
	 * Get the pointer of the body region with the size check of the buffer.
	 *
	 * @tparam HeaderType A header type.
	 * @tparam BodyType
	 * A Body type. If a body doesn't exist, 'void' shall be set.
	 *
	 * @param buf A buffer for the packet.
	 * @param additionalSize An additional content size.
	 *
	 * @return
	 * An address next to the header region. It is typically the top of
	 * the body.
	 */
	template<class HeaderType, class BodyType>
	BodyType *getBodyPointerWithCheck(const mlpl::SmartBuffer &buf,
	                                  const size_t &additionalSize = 0)
	  throw(HatoholException)
	{
		const size_t requiredSize = sizeof(HeaderType)
		                            + getBodySize<BodyType>()
		                            + additionalSize;
		if (buf.size() < requiredSize) {
			THROW_HATOHOL_EXCEPTION(
			  "Bad size: required: %zd, buffer: %zd (%s, %s)",
			  requiredSize, buf.size(),
			  DEMANGLED_TYPE_NAME(HeaderType),
			  DEMANGLED_TYPE_NAME(BodyType));
		}
		return buf.getPointer<BodyType>(sizeof(HeaderType));
	}

	template<class BodyType>
	size_t getBodySize(void)
	{
		return sizeof(BodyType);
	}

	/**
	 * Allocate buffer for the command, Setup the header, and
	 * return the top address for the body.
	 *
	 * @tparam BodyType
	 * A Body type. If a body doesn't exist, 'void' shall be set.
	 *
	 * @param cmdBuf A buffer for the command.
	 * @param code   A command code.
	 * @param additionalSize An additional content size.
	 *
	 * @return
	 * An address next to the header region. It is typically the top of
	 * the body.
	 */
	template<class BodyType>
	BodyType *setupCommandHeader(mlpl::SmartBuffer &cmdBuf,
	                             const HapiCommandCode &code,
	                             const size_t &additionalSize = 0)
	{
		const size_t requiredSize = sizeof(HapiCommandHeader)
		                            + getBodySize<BodyType>()
		                            + additionalSize;
		cmdBuf.alloc(requiredSize);
		HapiCommandHeader *cmdHeader =
		  cmdBuf.getPointer<HapiCommandHeader>(0);
		cmdHeader->type = NtoL(HAPI_MSG_COMMAND);
		cmdHeader->code = NtoL(code);
		cmdHeader->sequenceId = NtoL(getIncrementedSequenceId());
		return cmdBuf.getPointer<BodyType>(sizeof(HapiCommandHeader));
	}

	/**
	 * Get the response body with a buffer size check.
	 *
	 * If the response size is smaller than the expected size,
	 * HatoholException is thrown.
	 *
	 * @tparam BodyType
	 * A Body type. If a body doesn't exist, 'void' shall be set.
	 *
	 * @param resBuf A response buffer.
	 * @param additionalSize An additional content size.
	 *
	 * @return
	 * An address next to the header region. It is typically the top of
	 * the body.
	 */
	template<class BodyType>
	BodyType *getResponseBody(mlpl::SmartBuffer &resBuf,
	                          const size_t &additionalSize = 0)
	  throw(HatoholException)
	{
		return getBodyPointerWithCheck<HapiResponseHeader, BodyType>(
		         resBuf, additionalSize);
	}

	/**
	 * Allocate buffer to include the specified body and set up
	 * parameters of the header.
	 *
	 * @tparam BodyType
	 * A Body type. If a body doesn't exist, 'void' shall be set.
	 *
	 *
	 * @param resBuf A SmartBuffer instance to be set up.
	 * @param additionalSize An addition size to allocate a buffer.
	 *
	 * @return A pointer of the body area.
	 */
	template<class BodyType>
	BodyType *setupResponseBuffer(
	  mlpl::SmartBuffer &resBuf,
	  const size_t &additionalSize = 0,
	  const HapiResponseCode &code = HAPI_RES_OK)
	{
		const size_t requiredSize = sizeof(HapiResponseHeader)
		                            + getBodySize<BodyType>()
		                            + additionalSize;
		resBuf.alloc(requiredSize);
		HapiResponseHeader *header =
		  resBuf.getPointer<HapiResponseHeader>(0);
		header->type = NtoL(HAPI_MSG_RESPONSE);
		header->code = NtoL(code);
		header->sequenceId = NtoL(getSequenceIdInProgress());
		return resBuf.getPointer<BodyType>(sizeof(HapiResponseHeader));
	}

	uint32_t getIncrementedSequenceId(void);
	void setSequenceId(const uint32_t &sequenceId);
	uint32_t getSequenceIdInProgress(void);

	void dumpBuffer(const mlpl::SmartBuffer &sbuf,
	                const std::string &label = "");

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

template <>
inline size_t HatoholArmPluginInterface::getBodySize<void>(void)
{
	return 0;
}

#endif // HatoholArmPluginInterface_h
