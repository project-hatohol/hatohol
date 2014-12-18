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
#include "AMQPConnectionInfo.h"
#include "AMQPMessageHandler.h"
#include <unistd.h>
#include <Logger.h>
#include <StringUtils.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>

using namespace std;
using namespace mlpl;

static const time_t  DEFAULT_TIMEOUT  = 1;

class AMQPConnection {
public:
	AMQPConnection(const AMQPConnectionInfo &info)
	: m_info(info),
	  m_connection(NULL),
	  m_socket(NULL),
	  m_socketOpened(false),
	  m_channel(0),
	  m_envelope()
	{
		const char *virtualHost = getVirtualHost();
		if (string(virtualHost) == "/") {
			virtualHost = "";
		}
		MLPL_INFO("Broker URL: <%s://%s:%d/%s>\n",
			  getScheme(),
			  getHost(),
			  getPort(),
			  virtualHost);
	}

	~AMQPConnection()
	{
		amqp_destroy_envelope(&m_envelope);
		disposeConnection();
	}

	bool connect()
	{
		m_connection = amqp_new_connection();
		if (initializeConnection()) {
			return true;
		} else {
			disposeConnection();
			return false;
		}
	}

	bool isConnected()
	{
		return m_connection;
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
	const AMQPConnectionInfo &m_info;
	amqp_connection_state_t m_connection;
	amqp_socket_t *m_socket;
	bool m_socketOpened;
	amqp_channel_t m_channel;
	amqp_envelope_t m_envelope;

	const char *getScheme(void)
	{
		if (m_info.useTLS()) {
			return "amqps";
		} else {
			return "amqp";
		}
	}

	const char *getHost()
	{
		return m_info.getHost();
	}

	guint getPort()
	{
		return m_info.getPort();
	}

	const char *getUser()
	{
		return m_info.getUser();
	}

	const char *getPassword()
	{
		return m_info.getPassword();
	}

	const char *getVirtualHost(void)
	{
		return m_info.getVirtualHost();
	}

	const string &getQueueName()
	{
		return m_info.getQueueName();
	}

	time_t getTimeout(void)
	{
		return m_info.getTimeout();
	}

	const string &getTLSCertificatePath(void)
	{
		return m_info.getTLSCertificatePath();
	}

	const string &getTLSKeyPath(void)
	{
		return m_info.getTLSKeyPath();
	}

	const string &getTLSCACertificatePath(void)
	{
		return m_info.getTLSCACertificatePath();
	}

	bool isTLSVerifyEnabled(void)
	{
		return m_info.isTLSVerifyEnabled();
	}

	void logErrorResponse(const char *context, int status)
	{
		logErrorResponse(context, static_cast<amqp_status_enum>(status));
	}

	void logErrorResponse(const char *context, amqp_status_enum status)
	{
		if (status == AMQP_STATUS_OK)
			return;
		MLPL_ERR("failed to %s: %d: %s\n",
			 context,
			 status,
			 amqp_error_string2(status));
	}

	void logErrorResponse(const char *context,
			      const amqp_rpc_reply_t &reply)
	{
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
	}

	bool initializeConnection()
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

	bool openSocket()
	{
		if (m_info.useTLS()) {
			if (!createTLSSocket()) {
				return false;
			}
		} else {
			if (!createPlainSocket()) {
				return false;
			}
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

		m_socketOpened = true;
		return true;
	}

	bool createTLSSocket(void)
	{
		m_socket = amqp_ssl_socket_new(m_connection);
		if (!m_socket) {
			MLPL_ERR("failed to create TLS socket\n");
			return false;
		}

		if (!setTLSKey()) {
			return false;
		}
		if (!setTLSCACertificate()) {
			return false;
		}
		if (!setTLSVerification()) {
			return false;
		}

		return true;
	}

	bool setTLSKey(void)
	{
		const int status =
			amqp_ssl_socket_set_key(
				m_socket,
				getTLSCertificatePath().c_str(),
				getTLSKeyPath().c_str());
		if (status != AMQP_STATUS_OK) {
			using StringUtils::sprintf;
			const string context =
				sprintf("set client certificate to SSL socket: "
					"<%s>:<%s>",
					getTLSCertificatePath().c_str(),
					getTLSKeyPath().c_str());
			logErrorResponse(context.c_str(), status);
			return false;
		}
		return true;
	}

	bool setTLSCACertificate(void)
	{
		const int status =
			amqp_ssl_socket_set_cacert(
				m_socket,
				getTLSCACertificatePath().c_str());
		if (status != AMQP_STATUS_OK) {
			using StringUtils::sprintf;
			const string context =
				sprintf("set CA certificate to SSL socket: <%s>",
					getTLSCACertificatePath().c_str());
			logErrorResponse(context.c_str(), status);
			return false;
		}
		return true;
	}

	bool setTLSVerification(void)
	{
		amqp_ssl_socket_set_verify(m_socket, isTLSVerifyEnabled());
		return true;
	}

	bool createPlainSocket(void)
	{
		m_socket = amqp_tcp_socket_new(m_connection);
		if (!m_socket) {
			MLPL_ERR("failed to create TCP socket\n");
			return false;
		}
		return true;
	}

	bool login()
	{
		const int heartbeat = 1;
		const amqp_rpc_reply_t reply =
			amqp_login(m_connection,
				   getVirtualHost(),
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
	}

	bool openChannel()
	{
		amqp_channel_open_ok_t *response;
		m_channel = 1;
		response = amqp_channel_open(m_connection, m_channel);
		if (!response) {
			const amqp_rpc_reply_t reply =
				amqp_get_rpc_reply(m_connection);
			logErrorResponse("open channel", reply);
			m_channel = 0;
			return false;
		}
		return true;
	}

	bool declareQueue()
	{
		const string &queueName = getQueueName();
		const amqp_bytes_t queue = amqp_cstring_bytes(queueName.c_str());
		MLPL_INFO("Queue: <%s>\n", queueName.c_str());
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
	}

	void disposeConnection()
	{
		if (!m_connection)
			return;

		disposeChannel();
		disposeSocket();
		logErrorResponse("destroy connection",
				 amqp_destroy_connection(m_connection));

		m_socket = NULL;
		m_connection = NULL;
	}

	void disposeChannel()
	{
		if (m_channel == 0)
			return;

		logErrorResponse("close channel",
				 amqp_channel_close(m_connection, m_channel,
						    AMQP_REPLY_SUCCESS));
		m_channel = 0;
	}

	void disposeSocket()
	{
		if (!m_socketOpened)
			return;

		logErrorResponse("close connection",
				 amqp_connection_close(m_connection,
						       AMQP_REPLY_SUCCESS));
		m_socketOpened = false;
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
	AMQPConnection connection(m_connectionInfo);
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
