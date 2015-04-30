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

#include "AMQPConsumer.h"
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

class AMQPConsumerConnection : public AMQPConnection {
public:
	AMQPConsumerConnection(const AMQPConnectionInfo &info)
	: AMQPConnection(info),
	  m_connection(NULL),
	  m_envelope()
	{
	}

	~AMQPConsumerConnection()
	{
		amqp_destroy_envelope(&m_envelope);
	}

	bool consume(amqp_envelope_t *&envelope)
	{
		amqp_maybe_release_buffers(m_connection);
		amqp_destroy_envelope(&m_envelope);

		struct timeval timeout = {
			getTimeout(),
			0
		};
		const int flags = 0;
		amqp_rpc_reply_t reply = amqp_consume_message(m_connection,
							      &m_envelope,
							      &timeout,
							      flags);
		switch (reply.reply_type) {
		case AMQP_RESPONSE_NORMAL:
			break;
		case AMQP_RESPONSE_LIBRARY_EXCEPTION:
			if (reply.library_error != AMQP_STATUS_TIMEOUT) {
				logErrorResponse("consume message", reply);
				disposeConnection();
			}
			return false;
		default:
			logErrorResponse("consume message", reply);
			disposeConnection();
			return false;
		}

		envelope = &m_envelope;
		return true;
	}

private:
	amqp_connection_state_t m_connection;
	amqp_envelope_t m_envelope;

	bool initializeConnection() override
	{
		if (!openSocket())
			return false;
		if (!login())
			return false;
		if (!openChannel())
			return false;
		if (!declareQueue())
			return false;
		if (!startConsuming())
			return false;
		return true;
	}

	bool startConsuming()
	{
		const amqp_bytes_t queue =
			amqp_cstring_bytes(getQueueName().c_str());
		const amqp_bytes_t consumer_tag = amqp_empty_bytes;
		const amqp_boolean_t no_local = false;
		const amqp_boolean_t no_ack = true;
		const amqp_boolean_t exclusive = false;
		const amqp_table_t arguments = amqp_empty_table;
		const amqp_basic_consume_ok_t *response;
		response = amqp_basic_consume(m_connection,
					      getChannel(),
					      queue,
					      consumer_tag,
					      no_local,
					      no_ack,
					      exclusive,
					      arguments);
		if (!response) {
			const amqp_rpc_reply_t reply =
				amqp_get_rpc_reply(m_connection);
			if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
				logErrorResponse("start consuming", reply);
				return false;
			}
		}
		return true;
	}
};

AMQPConsumer::AMQPConsumer(const AMQPConnectionInfo &connectionInfo,
			   AMQPMessageHandler *handler)
: m_connectionInfo(connectionInfo),
  m_handler(handler)
{
}

AMQPConsumer::~AMQPConsumer()
{
}

gpointer AMQPConsumer::mainThread(HatoholThreadArg *arg)
{
	AMQPConsumerConnection connection(m_connectionInfo);
	while (!isExitRequested()) {
		if (!connection.isConnected()) {
			connection.connect();
		}

		if (!connection.isConnected()) {
			sleep(1); // TODO: Make retry interval customizable
			continue;
		}

		amqp_envelope_t *envelope;
		const bool consumed = connection.consume(envelope);
		if (!consumed)
			continue;

		m_handler->handle(envelope);
	}
	return NULL;
}
