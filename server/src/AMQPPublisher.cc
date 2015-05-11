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

#include "AMQPPublisher.h"
#include "AMQPConnection.h"
#include "AMQPConnectionInfo.h"
#include "AMQPMessageHandler.h"
#include <unistd.h>
#include <Logger.h>
#include <StringUtils.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>

using namespace std;
using namespace mlpl;

class AMQPPublisherConnection : public AMQPConnection {
public:
	AMQPPublisherConnection(const AMQPConnectionInfo &info)
	: AMQPConnection(info)
	{
	}

	~AMQPPublisherConnection()
	{
	}

	bool publish(string &body)
	{
		const amqp_bytes_t exchange = amqp_empty_bytes;
		const amqp_bytes_t queue =
			amqp_cstring_bytes(getQueueName().c_str());
		const amqp_boolean_t no_mandatory = false;
		const amqp_boolean_t no_immediate = false;
		int response;
		amqp_basic_properties_t props;
		props._flags =
			AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
		props.content_type = amqp_cstring_bytes("application/json");
		props.delivery_mode = 2;
		amqp_bytes_t body_bytes;
		body_bytes.bytes = const_cast<char *>(body.data());
		body_bytes.len = body.length();
		response = amqp_basic_publish(getConnection(),
					      getChannel(),
					      exchange,
					      queue,
					      no_mandatory,
					      no_immediate,
					      &props,
					      amqp_bytes_malloc_dup(body_bytes));
		if (response != AMQP_STATUS_OK) {
			const amqp_rpc_reply_t reply =
				amqp_get_rpc_reply(getConnection());
			if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
				logErrorResponse("start publishing", reply);
				return false;
			}
		}
		return true;
	}
};

AMQPPublisher::AMQPPublisher(const AMQPConnectionInfo &connectionInfo,
			     string body)
: m_connectionInfo(connectionInfo),
  m_body(body)
{
}

AMQPPublisher::~AMQPPublisher()
{
}

bool AMQPPublisher::publish(void)
{
	AMQPPublisherConnection connection(m_connectionInfo);
	if (!connection.isConnected()) {
		connection.connect();
	}
	return connection.publish(m_body);
}
