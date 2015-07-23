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
#ifndef AMQPConnection_h
#define AMQPConnection_h

#include "AMQPConnectionInfo.h"
#include <glib.h>
#include <unistd.h>
#include <Logger.h>
#include <StringUtils.h>
#include <UsedCountable.h>
#include <UsedCountablePtr.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>

class AMQPConnection;
typedef UsedCountablePtr<AMQPConnection> AMQPConnectionPtr;

struct AMQPMessage {
	std::string contentType;
	std::string body;
};

struct AMQPJSONMessage : public AMQPMessage {
	AMQPJSONMessage()
	{
		contentType = "application/json";
	}
};

class AMQPConnection : public UsedCountable
{
public:
	static int DEFAULT_HEARTBEAT_SECONDS;

	static AMQPConnectionPtr create(const AMQPConnectionInfo &info);

	AMQPConnection(const AMQPConnectionInfo &info);
	~AMQPConnection();
	const AMQPConnectionInfo &getConnectionInfo(void);
	bool connect(void);
	bool isConnected(void);
	bool startConsuming(void);
	bool consume(AMQPMessage &message);
	bool publish(const AMQPMessage &message);
	bool purgeAllQueues(void);
	bool deleteAllQueues(void);

protected:
	bool initializeConnection(void);
	time_t getTimeout(void);
	bool openSocket(void);
	bool login(void);
	bool openChannel(void);
	bool declareQueue(const std::string queueName);
	bool purgeQueue(const std::string queueName);
	bool deleteQueue(const std::string queueName);
	std::string getConsumerQueueName(void);
	std::string getPublisherQueueName(void);
	void disposeConnection(void);
	void logErrorResponse(const char *context,
			      const amqp_rpc_reply_t &reply);
	amqp_channel_t getChannel(void);
	amqp_connection_state_t getConnection(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // AMQPConnection_h
