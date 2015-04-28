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
#include "AMQPMessageHandler.h"
#include <glib.h>
#include <unistd.h>
#include <Logger.h>
#include <StringUtils.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>

class AMQPConnection {
public:
	AMQPConnection(const AMQPConnectionInfo &info);
	~AMQPConnection();
	virtual bool connect();
	virtual bool initializeConnection();
	virtual time_t getTimeout(void);
	virtual bool isConnected();
	virtual bool openSocket();
	virtual bool login();
	virtual bool openChannel();
	virtual bool declareQueue();
	virtual std::string getQueueName();
	virtual void disposeConnection();
	virtual void logErrorResponse(const char *context,
	                              const amqp_rpc_reply_t &reply);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // AMQPConnection_h
