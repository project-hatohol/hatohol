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

#include "AMQPConsumer.h"
#include "AMQPMessageHandler.h"
#include <Logger.h>
#include <StringUtils.h>
#include <amqp_tcp_socket.h>
#include <libsoup/soup.h>

using namespace std;
using namespace mlpl;

static const time_t  DEFAULT_TIMEOUT  = 1;
static const char   *DEFAULT_USER     = "guest";
static const char   *DEFAULT_PASSWORD = "guest";

class AMQPConnection {
public:
	AMQPConnection(const string &brokerUrl,
		       const string &queueAddress)
	: m_brokerUrl(soup_uri_new(buildURL(brokerUrl).c_str())),
	  m_queueAddress(queueAddress),
	  m_timeout(DEFAULT_TIMEOUT),
	  m_connection(NULL),
	  m_socket(NULL),
	  m_channel(1),
	  m_envelope()
	{
	};

	~AMQPConnection()
	{
		soup_uri_free(m_brokerUrl);
		amqp_destroy_envelope(&m_envelope);
		disposeConnection();
	};

	bool connect()
	{
		m_connection = amqp_new_connection();
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
	};

	bool consume(amqp_envelope_t *&envelope) {
		amqp_maybe_release_buffers(m_connection);
		amqp_destroy_envelope(&m_envelope);

		struct timeval timeout = {
			m_timeout,
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
			}
			return false;
		default:
			logErrorResponse("consume message", reply);
			return false;
		}

		envelope = &m_envelope;
		return true;
	};


private:
	SoupURI *m_brokerUrl;
	string m_queueAddress;
	time_t m_timeout;
	amqp_connection_state_t m_connection;
	amqp_socket_t *m_socket;
	amqp_channel_t m_channel;
	amqp_envelope_t m_envelope;

	string buildURL(const string brokerUrl) {
		return string("amqp://") + brokerUrl;
	};

	const char *getHost() {
		return soup_uri_get_host(m_brokerUrl);
	};

	guint getPort() {
		return soup_uri_get_port(m_brokerUrl);
	};

	const char *getUser() {
		const char *user;
		user = soup_uri_get_user(m_brokerUrl);
		if (!user) {
			user = DEFAULT_USER;
		}
		return user;
	};

	const char *getPassword() {
		const char *password;
		password = soup_uri_get_password(m_brokerUrl);
		if (!password) {
			password = DEFAULT_PASSWORD;
		}
		return password;
	};

	void logErrorResponse(const char *context, int status) {
		logErrorResponse(context, static_cast<amqp_status_enum>(status));
	};

	void logErrorResponse(const char *context, amqp_status_enum status) {
		if (status == AMQP_STATUS_OK)
			return;
		// TODO: stringify status
		MLPL_ERR("failed to %s: %x\n", context, status);
	};

	void logErrorResponse(const char *context,
			      const amqp_rpc_reply_t &reply) {
		switch (reply.reply_type) {
		case AMQP_RESPONSE_NORMAL:
			break;
		case AMQP_RESPONSE_NONE:
			MLPL_ERR("%s: RPC reply type is missing\n", context);
			break;
		case AMQP_RESPONSE_LIBRARY_EXCEPTION:
			MLPL_ERR("failed to %s: %s\n",
				 context,
				 amqp_error_string2(reply.library_error));
			break;
		case AMQP_RESPONSE_SERVER_EXCEPTION:
			// TODO: show more messages
			MLPL_ERR("%s: server exception\n",
				 context);
			break;
		}
		return;
	};

	bool openSocket() {
		m_socket = amqp_tcp_socket_new(m_connection);
		if (!m_socket) {
			MLPL_ERR("failed to create TCP socket\n");
			return false;
		}

		const int status =
			amqp_socket_open(m_socket, getHost(), getPort());
		if (status != AMQP_STATUS_OK) {
			const string context =
				StringUtils::sprintf("open socket: %s:%u",
						     getHost(),
						     getPort());
			logErrorResponse(context.c_str(), status);
			return false;
		}

		return true;
	};

	bool login() {
		const char *vhost = "/";
		const int heartbeat = 1;
		const amqp_rpc_reply_t reply =
			amqp_login(m_connection,
				   vhost,
				   AMQP_DEFAULT_MAX_CHANNELS,
				   AMQP_DEFAULT_FRAME_SIZE,
				   heartbeat,
				   AMQP_SASL_METHOD_PLAIN,
				   getUser(),
				   getPassword());
		if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
			const string context =
				StringUtils::sprintf("login with <%s>",
						     getUser());
			logErrorResponse(context.c_str(), reply);
			return false;
		}
		return true;
	};

	bool openChannel() {
		amqp_channel_open_ok_t *response;
		response = amqp_channel_open(m_connection, m_channel);
		if (!response) {
			const amqp_rpc_reply_t reply =
				amqp_get_rpc_reply(m_connection);
			logErrorResponse("open channel", reply);
			return false;
		}
		return true;
	};

	bool declareQueue() {
		const amqp_bytes_t queue =
			amqp_cstring_bytes(m_queueAddress.c_str());
		MLPL_INFO("Queue: <%s>\n", m_queueAddress.c_str());
		const amqp_boolean_t passive = false;
		const amqp_boolean_t durable = false;
		const amqp_boolean_t exclusive = false;
		const amqp_boolean_t auto_delete = false;
		const amqp_table_t arguments = amqp_empty_table;
		const amqp_queue_declare_ok_t *response =
			amqp_queue_declare(m_connection,
					   m_channel,
					   queue,
					   passive,
					   durable,
					   exclusive,
					   auto_delete,
					   arguments);
		if (!response) {
			const amqp_rpc_reply_t reply =
				amqp_get_rpc_reply(m_connection);
			if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
				logErrorResponse("declare queue", reply);
				return false;
			}
		}
		return true;
	};

	bool startConsuming() {
		const amqp_bytes_t queue =
			amqp_cstring_bytes(m_queueAddress.c_str());
		const amqp_bytes_t consumer_tag = amqp_empty_bytes;
		const amqp_boolean_t no_local = false;
		const amqp_boolean_t no_ack = true;
		const amqp_boolean_t exclusive = false;
		const amqp_table_t arguments = amqp_empty_table;
		const amqp_basic_consume_ok_t *response;
		response = amqp_basic_consume(m_connection,
					      m_channel,
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
	};

	void disposeConnection() {
		if (!m_connection)
			return;

		logErrorResponse("close channel",
				 amqp_channel_close(m_connection, m_channel,
						    AMQP_REPLY_SUCCESS));
		logErrorResponse("close connection",
				 amqp_connection_close(m_connection,
						       AMQP_REPLY_SUCCESS));
		logErrorResponse("destroy connection",
				 amqp_destroy_connection(m_connection));
	};
};

AMQPConsumer::AMQPConsumer(const string &brokerUrl,
			   const string &queueAddress,
			   AMQPMessageHandler *handler)
: m_brokerUrl(brokerUrl),
  m_queueAddress(queueAddress),
  m_handler(handler)
{
}

AMQPConsumer::~AMQPConsumer()
{
}

gpointer AMQPConsumer::mainThread(HatoholThreadArg *arg)
{
	AMQPConnection connection(m_brokerUrl, m_queueAddress);
	if (!connection.connect()) {
		// TODO: Support reconnecting.
		return NULL;
	}

	while (!isExitRequested()) {
		amqp_envelope_t *envelope;
		const bool consumed = connection.consume(envelope);
		if (!consumed)
			continue;

		m_handler->handle(envelope);
	}
	return NULL;
}
