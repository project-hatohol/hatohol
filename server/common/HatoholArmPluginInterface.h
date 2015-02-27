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

#ifndef HatoholArmPluginInterface_h
#define HatoholArmPluginInterface_h

#include <string>
#include <SmartBuffer.h>
#include <EndianConverter.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Address.h>
#include "HatoholThreadBase.h"
#include "HatoholException.h"
#include "ItemDataPtr.h"
#include "ItemGroupPtr.h"
#include "ItemTablePtr.h"
#include "Utils.h"
#include "MonitoringServerInfo.h"

enum HatoholArmPluginErrorCode {
	HAPERR_OK,
	HAPERR_NOT_FOUND_QUEUE_ADDR,
	HAPERR_UNAVAILABLE_HAP,
	HAPERR_UNKNOWN,
};

struct HatoholArmPluginError {
	HatoholArmPluginErrorCode code;
};

class HapInitiatedException : public std::exception {
};

typedef enum {
	COLLECT_NG_PARSER_ERROR = 0,
	COLLECT_NG_DISCONNECT_ZABBIX,
	COLLECT_NG_DISCONNECT_NAGIOS,
	COLLECT_NG_PLGIN_INTERNAL_ERROR,
	COLLECT_NG_HATOHOL_INTERNAL_ERROR,
	COLLECT_NG_PLGIN_CONNECT_ERROR,
	COLLECT_NG_AMQP_CONNECT_ERROR,
	NUM_COLLECT_NG_KIND,
	COLLECT_OK,
} HatoholArmPluginWatchType;

enum HapiMessageType {
	HAPI_MSG_INITIATION,          // Sv -> Cl
	HAPI_MSG_INITIATION_RESPONSE, // Cl -> Sv
	HAPI_MSG_INITIATION_FINISH,   // Sv -> Cl (confirmation)
	HAPI_MSG_INITIATION_REQUEST,  // Cl -> Sv
	HAPI_MSG_COMMAND,
	HAPI_MSG_RESPONSE,
	NUM_HAPI_MSG
};

enum HapiCommandCode {
	// Cl -> Sv
	HAPI_CMD_GET_MONITORING_SERVER_INFO,
	HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER,
	HAPI_CMD_GET_LAST_EVENT_ID,
	HAPI_CMD_GET_TIME_OF_LAST_EVENT,
	HAPI_CMD_SEND_UPDATED_TRIGGERS,
	HAPI_CMD_SEND_HOSTS,
	HAPI_CMD_SEND_HOST_GROUP_ELEMENTS,
	HAPI_CMD_SEND_HOST_GROUPS,
	HAPI_CMD_SEND_UPDATED_EVENTS,
	HAPI_CMD_SEND_ARM_INFO,
	HAPI_CMD_SEND_HAP_SELF_TRIGGERS,
	// Sv -> Cl
	HAPI_CMD_REQ_FETCH_ITEMS,
	HAPI_CMD_REQ_TERMINATE,

	// Since 14.12
	// Sv -> Cl
	HAPI_CMD_REQ_FETCH_HISTORY,

	// Since 15.03
	// Sv -> Cl
	HAPI_CMD_REQ_FETCH_TRIGGERS,
	// Cl -> Sv
	HAPI_CMD_SEND_ALL_TRIGGERS,
	HAPI_CMD_GET_TRIGGERS_COLLECT_TYPE,

	// Sv -> Cl
	NUM_HAPI_CMD
};

enum HapiResponseCode {
	HAPI_RES_OK,
	HAPI_RES_INVALID_HEADER,
	HAPI_RES_UNKNOWN_CODE,
	HAPI_RES_INVALID_ARG,
	HAPI_RES_UNEXPECTED_SEQ_ID,
	HAPI_RES_ERR_DESTRUCTED,

	// Cl -> Sv
	HAPI_RES_ITEMS,

	// Since 14.12
	// Cl -> Sv
	HAPI_RES_HISTORY,

	// Since 15.03
	// Cl -> Sv
	HAPI_RES_TRIGGERS,

	NUM_HAPI_CMD_RES
};

typedef enum {
	DIFFERENCE_TRIGGER_COLLECT = 0,
	ALL_TRIGGER_COLLECT,
} HapiTriggerCollectType;

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

struct HapiItemTableHeader {
	uint16_t flags;
	uint32_t numGroups;
	uint32_t length;
	// HapiItemGroupHeader
	// HapiItemDataHeader ...
	// HapiItemGroupHeader
	// HapiItemDataHeader ...
	// ...
} __attribute__((__packed__));

struct HapiItemGroupHeader {
	uint16_t flags;
	uint32_t numItems;

	// Total bytes of this header and all of the ItemData.
	uint32_t length;
} __attribute__((__packed__));

#define HAPI_ITEM_DATA_HEADER_FLAG_NULL 0x01

struct HapiItemDataHeader {
	//   0b: Null flag (0: Not NULL, 1: NULL)
	// 1-7b: reseverd
	uint8_t  flags;

	// 0: ITEM_TYPE_BOOL
	// 1: ITEM_TYPE_INT
	// 2: ITEM_TYPE_UINT64
	// 3: ITEM_TYPE_DOUBLE
	// 4: ITEM_TYPE_STRING
	uint8_t  type;

	uint64_t itemId;

	// Data Body: Field size is the following.
	//   1B (BOOL)
	//   8B (INT)
	//   8B (UINT64)
	//   8B (DOUBLE: IEEE754 [64bit])

} __attribute__((__packed__));

struct HapiItemStringHeader {
	HapiItemDataHeader dataHeader;
	uint32_t           length;  // not count a NULL terminator.
	// string body: NULL terminator is needed.
} __attribute__((__packed__));

struct HapiArmInfo {
	uint8_t  running;
	uint8_t  stat;
	uint64_t statUpdateTime;
	uint32_t statUpdateTimeNanosec;
	uint16_t failureCommentLength; // Not include the NULL terminator
	uint16_t failureCommentOffset; // from the top of this structure
	uint64_t failureReason;
	uint64_t lastSuccessTime;
	uint32_t lastSuccessTimeNanosec;
	uint64_t lastFailureTime;
	uint32_t lastFailureTimeNanosec;
	uint64_t numUpdate;
	uint64_t numFailure;
} __attribute__((__packed__));

struct HapiParamTimeOfLastEvent {
	uint64_t triggerId;
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
	uint16_t userNameLength;  // Not include the NULL terminator
	uint16_t userNameOffset;  // from the top of this structure
	uint16_t passwordLength;  // Not include the NULL terminator
	uint16_t passwordOffset;  // from the top of this structure
	uint16_t dbNameLength;    // Not include the NULL terminator
	uint16_t dbNameOffset;    // from the top of this structure
	uint32_t port;
	uint32_t pollingIntervalSec;
	uint32_t retryIntervalSec;
	// Body of hostName  (including NULL terminator)
	// Body of ipAddress (including NULL terminator)
	// Body of nickname  (including NULL terminator)
	// Body of userName  (including NULL terminator)
	// Body of password  (including NULL terminator)
	// Body of dbName    (including NULL terminator)
} __attribute__((__packed__));

struct HapiResTimestampOfLastTrigger {
	uint64_t timestamp; // Unix time (GMT)
	uint32_t nanosec;
} __attribute__((__packed__));

struct HapiResLastEventId {
	uint64_t lastEventId;
} __attribute__((__packed__));

struct HapiResTimeOfLastEvent {
	uint64_t sec;
	uint32_t nsec;
} __attribute__((__packed__));

struct HapiHapSelfTriggers {
	uint64_t numTriggers;
	// Traling data is an array of HatoholArmPluginWatchType.
} __attribute__((__packed__));

struct HapiParamReqFetchHistory {
	uint64_t hostId;
	uint64_t itemId;
	uint16_t valueType;
	uint64_t beginTime;
	uint64_t endTime;
} __attribute__((__packed__));

struct HapiTriggerCollect {
	uint64_t type;
} __attribute__((__packed__));

class HatoholArmPluginInterface :
  public HatoholThreadBase, public EndianConverter {
public:
	struct MessagingContext {
		qpid::messaging::Address replyAddress;
		uint32_t sequenceId;
	};

	static const char *ENV_NAME_QUEUE_ADDR;
	static const char *DEFAULT_BROKER_URL;
	static const uint32_t SEQ_ID_UNKNOWN;
	static const uint32_t SEQ_ID_MAX;

	typedef void (HatoholArmPluginInterface::*CommandHandler)(
	  const HapiCommandHeader *header);

	class CommandCallbacks : public UsedCountable {
	public:
		virtual void onGotReply(mlpl::SmartBuffer &replyBuf,
		                        const HapiCommandHeader &cmdHeader);
		virtual void onError(const HapiResponseCode &code,
		                     const HapiCommandHeader &cmdHeader);
	protected:
		virtual ~CommandCallbacks();
	};
	typedef UsedCountablePtr<CommandCallbacks> CommandCallbacksPtr;

	HatoholArmPluginInterface(
	  const bool &workInServer = false);
	virtual ~HatoholArmPluginInterface() override;

	virtual void exitSync(void) override;

	void send(const std::string &message);

	/**
	 * Send data to the broker's queue.
	 *
	 * @param smbuf Data to be sent.
	 * @param callbacks
	 * An instance including methods that are called back when the reply
	 * comes or gets an error. If this paramter is NULL and the data type
	 * is HAPI_MSG_COMMAND, the default callback handlers (mainly works to
	 * keep the internal consistency) are created and used.
	 */
	void send(
	  const mlpl::SmartBuffer &smbuf, CommandCallbacks *callbacks = NULL);

	bool getMessagingContext(MessagingContext &msgCtx);
	void reply(const mlpl::SmartBuffer &replyBuf);
	void reply(const MessagingContext &msgCtx,
	           const mlpl::SmartBuffer &replyBuf);
	void replyError(const HapiResponseCode &code);
	void replyOk(void);

	bool isConnetced(void);

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

	/**
	 * Append HapiItemTableHeader to the SmartBuffer.
	 *
	 * Note that: completeItemTable() shall be called after all ItemGroup
	 * instances are appended.
	 *
	 * @param sbuf
	 * A SmartBuffer instance for appending HapiItemTableHeader data.
	 * The buffer size is automatically extended if necessary.
	 *
	 * @param numGroups The number of groups the table has.
	 *
	 * @return The index of the top of the appended header.
	 */
	static size_t appendItemTableHeader(mlpl::SmartBuffer &sbuf,
	                                    const size_t &numGroups);

	/**
	 * Complete an ItemTable on the buffer.
	 *
	 * @param sbuf
	 * A SmartBuffer instance for appending HapiItemTableHeader data.
	 *
	 * @param headerIndex
	 * A top index of the HapiItemTableHeader that is to be completed.
	 */
	static void completeItemTable(mlpl::SmartBuffer &sbuf,
	                              const size_t &headerIndex);

	/**
	 * Append HapiTable to the SmartBuffer.
	 *
	 * @param sbuf
	 * A SmartBuffer instance for appending HapiItemTable data.
	 * The buffer size is automatically extended if necessary.
	 *
	 * @param itemTablePtr An ItemTable to be appended.
	 */
	static void appendItemTable(mlpl::SmartBuffer &sbuf,
	                            ItemTablePtr itemTablePtr);

	/**
	 * Append HapiItemGroupHeader to the SmartBuffer.
	 *
	 * Note that: completeItemGroup() shall be called after all ItemData
	 * instances are appended.
	 *
	 * @param sbuf
	 * A SmartBuffer instance for appending HapiItemGroupHeader data.
	 * The buffer size is automatically extended if necessary.
	 *
	 * @param numItems The number of items the group has.
	 *
	 * @return The index of the top of the append header.
	 */
	static size_t appendItemGroupHeader(mlpl::SmartBuffer &sbuf,
	                                    const size_t &numItems);
	/**
	 * Append HapiItem to the SmartBuffer.
	 *
	 * @param sbuf
	 * A SmartBuffer instance for appending HapiItemGroupHeader data.
	 * The buffer size is automatically extended if necessary.
	 *
	 * @param itemGrpPtr An ItemGroup to be appended.
	 */
	static void appendItemGroup(mlpl::SmartBuffer &sbuf,
	                            ItemGroupPtr itemGrpPtr);
	/**
	 * Complete an ItemGroup on the buffer.
	 *
	 * @param sbuf
	 * A SmartBuffer instance for appending HapiItemGroupHeader data.
	 *
	 * @param headerIndex
	 * A top index of the HapiItemGroupHeader that is to be completed.
	 */
	static void completeItemGroup(mlpl::SmartBuffer &sbuf,
	                              const size_t &headerIndex);

	/**
	 * Append HapiItemData to the SmartBuffer.
	 *
	 * @param sbuf
	 * A SmartBuffer instance for appending the data. The buffer size is
	 * automatically extended if necessary.
	 * After this method is created, the index of 'sbuf' is forwarded.
	 *
	 * @param itemData An ItemData to be written.
	 */
	static void appendItemData(mlpl::SmartBuffer &sbuf,
	                           ItemDataPtr itemData);

	/**
	 * Create an ItemTable instance and append the subsequent
	 * ItemGroup and ItemData instances from the buffer data.
	 *
	 * @param sbuf
	 * A SmartBuffer instance. The index shall be at the top of
	 * the HapiItemTableHeader region followed by HapiItemDataHeaders
	 * and HapiItemGroupHeaders of the targert.
	 * After this method is called, the index of 'sbuf' is forwarded.
	 *
	 * @return A created ItemGroup.
	 */
	static ItemTablePtr createItemTable(mlpl::SmartBuffer &sbuf)
	  throw(HatoholException);

	/**
	 * Create an ItemGroup instance push ItemData instances from
	 * the buffer data.
	 *
	 * @param sbuf
	 * A SmartBuffer instance. The index shall be at the top of
	 * the HapiItemGroupHeader region followed by HapiItemDataHeaders
	 * of the targert.
	 * After this method is called, the index of 'sbuf' is forwarded.
	 *
	 * @return A created ItemGroup.
	 */
	static ItemGroupPtr createItemGroup(mlpl::SmartBuffer &sbuf)
	  throw(HatoholException);

	/**
	 * Create the ItemData instance from the buffer data.
	 *
	 * @param sbuf
	 * A SmartBuffer instance. The index shall be at the top of
	 * the HapiItemDataHeader region of the targert.
	 * After this method is created, the index of 'sbuf' is forwarded.
	 *
	 * @return A created ItemData.
	 */
	static ItemDataPtr createItemData(mlpl::SmartBuffer &sbuf)
	  throw(HatoholException);

	std::string getBrokerUrl(void) const;
	void setBrokerUrl(const std::string &brokerUrl);

	std::string getQueueAddress(void) const;
	void setQueueAddress(const std::string &queueAddr);

	/**
	 * Set the GLibMainContext.
	 *
	 * @param context A GLib's main context.
	 * This method internally calls g_main_context_ref() to this object.
	 * If any context is already set, it is replaced and
	 * g_main_context_unref() is called for it.
	 */
	void setGLibMainContext(GMainContext *context);

	/**
	 * Get the GLibMainContext set by setGLibMainContext().
	 *
	 * By default, none of GLib's main context is not set to this object.
	 * I.e. this method returns NULL.
	 * This method doesn't change the reference count of the returned
	 * object.
	 *
	 * @return A GLib's main context.
	 */
	GMainContext *getGLibMainContext(void) const;

protected:
	typedef std::map<uint16_t, CommandHandler> CommandHandlerMap;
	typedef CommandHandlerMap::iterator        CommandHandlerMapIterator;
	typedef CommandHandlerMap::const_iterator  CommandHandlerMapConstIterator;

	virtual gpointer mainThread(HatoholThreadArg *arg) override;

	/**
	 * Called when starting the mainThread.
	 */
	virtual void onSetPluginInitialInfo(void);

	/**
	 * Called when connection with the AMQP broker is established.
	 *
	 * @param conn A connection object
	 */
	virtual void onConnected(qpid::messaging::Connection &conn);

	/**
	 * Called when connection with the AMQP broker is failure.
	 */
	virtual void onFailureConnected(void);

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
	virtual void onHandledCommand(const HapiCommandCode &code);

	/**
	 * Called when a HAPI's response is received.
	 *
	 * @param smbuf
	 * A received response. The index of smbuf has been reset to zero.
	 */
	virtual void onGotResponse(const HapiResponseHeader *header,
	                           mlpl::SmartBuffer &resBuf);

	/**
	 * Called when started to wait for a AMQP broker's response.
	 */
	virtual void onPriorToFetchMessage(void);

	/**
	 * Called when a AMQP broker's response is success.
	 */
	virtual void onSuccessFetchMessage(void);

	/**
	 * Called when a AMQP broker's response is failure.
	 */
	virtual void onFailureFetchMessage(void);

	/**
	 * Called when a HAPI's response is failure.
	 */
	virtual void onFailureReceivedMessage(void);

	void sendInitiationPacket(void);
	void sendInitiationRequest(void);
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
	 * @param cmdBuf
	 * A buffer for the command. The index is set to the nex to the header
	 * region after the call.
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
		cmdBuf.setIndex(sizeof(HapiCommandHeader));
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
	BodyType *getResponseBody(const mlpl::SmartBuffer &resBuf,
	                          const size_t &additionalSize = 0)
	  throw(HatoholException)
	{
		return getBodyPointerWithCheck<HapiResponseHeader, BodyType>(
		         resBuf, additionalSize);
	}

	/**
	 * Get the command body with a buffer size check.
	 *
	 * If the command buffer size is smaller than the expected size,
	 * HatoholException is thrown.
	 *
	 * @tparam BodyType
	 * A Body type. If a body doesn't exist, 'void' shall be set.
	 *
	 * @param cmdBuf A command buffer.
	 * @param additionalSize An additional content size.
	 *
	 * @return
	 * An address next to the header region. It is typically the top of
	 * the body.
	 */
	template<class BodyType>
	BodyType *getCommandBody(const mlpl::SmartBuffer &cmdBuf,
	                         const size_t &additionalSize = 0)
	  throw(HatoholException)
	{
		return getBodyPointerWithCheck<HapiCommandHeader, BodyType>(
		         cmdBuf, additionalSize);
	}

	/**
	 * Allocate buffer to include the specified body and set up
	 * parameters of the header.
	 *
	 * @tparam BodyType
	 * A Body type. If a body doesn't exist, 'void' shall be set.
	 *
	 * @param resBuf
	 * A SmartBuffer instance to be set up. The index is set to the next
	 * to the header region after the call.
	 *
	 * @param additionalSize An addition size to allocate a buffer.
	 * @param msgCtx
	 * A MessagingContext instance. It is necessary when this method
	 * is used asynchronously.
	 *
	 * @return A pointer of the body area.
	 */
	template<class BodyType>
	BodyType *setupResponseBuffer(
	  mlpl::SmartBuffer &resBuf,
	  const size_t &additionalSize = 0,
	  const HapiResponseCode &code = HAPI_RES_OK,
	  const MessagingContext *msgCtx = NULL)
	{
		const size_t requiredSize = sizeof(HapiResponseHeader)
		                            + getBodySize<BodyType>()
		                            + additionalSize;
		resBuf.alloc(requiredSize);
		HapiResponseHeader *header =
		  resBuf.getPointer<HapiResponseHeader>(0);
		header->type = NtoL(HAPI_MSG_RESPONSE);
		header->code = NtoL(code);
		header->sequenceId = NtoL(
		  msgCtx ? msgCtx->sequenceId : getSequenceIdInProgress());
		resBuf.setIndex(sizeof(HapiResponseHeader));
		return resBuf.getPointer<BodyType>(sizeof(HapiResponseHeader));
	}

	uint32_t getIncrementedSequenceId(void);
	void setSequenceId(const uint32_t &sequenceId);
	uint32_t getSequenceIdInProgress(void);

	HapiMessageType getMessageType(const mlpl::SmartBuffer &smbuf);

	/**
	 * Get the received buffer that is currently being processed.
	 * This method is seemed to be called from command handlers.
	 *
	 * @return
	 * A currently processed receive buffer. Or NULL if no buffer is
	 * processed.
	 */
	mlpl::SmartBuffer *getCurrBuffer(void);

	void dumpBuffer(const mlpl::SmartBuffer &sbuf,
	                const std::string &label = "");

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

template <>
inline size_t HatoholArmPluginInterface::getBodySize<void>(void)
{
	return 0;
}

#define HAP_PIPE_NAME_FMT "hap-pipe-%" FMT_SERVER_ID
#define HAP_PIPE_OPT      "hap-pipe"

#endif // HatoholArmPluginInterface_h
