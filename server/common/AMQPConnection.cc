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

#include "AMQPConnection.h"
#include <string.h>

using namespace std;
using namespace mlpl;

int AMQPConnection::DEFAULT_HEARTBEAT_SECONDS = 60;

struct AMQPConnection::Impl {
public:
	amqp_connection_state_t m_connection;
	bool m_isConsumerQueueDeclared;
	bool m_isPublisherQueueDeclared;
	const AMQPConnectionInfo m_info;
	amqp_socket_t *m_socket;
	bool m_socketOpened;
	amqp_channel_t m_channel;

	Impl(const AMQPConnectionInfo &info)
	: m_connection(NULL),
	  m_isConsumerQueueDeclared(false),
	  m_isPublisherQueueDeclared(false),
	  m_info(info),
	  m_socket(NULL),
	  m_socketOpened(false),
	  m_channel(0)
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

	~Impl()
	{
		disposeConnection();
	}


	bool isConnected()
	{
		return m_connection;
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
		if (status == AMQP_STATUS_SOCKET_ERROR) {
			const string context =
				StringUtils::sprintf("connect socket: %s:%u",
						     getHost(),
						     getPort());
			logErrorResponse(context.c_str(), status);
			return false;
		} else if (status != AMQP_STATUS_OK) {
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

	bool login()
	{
		const amqp_rpc_reply_t reply =
			amqp_login(m_connection,
				   getVirtualHost(),
				   AMQP_DEFAULT_MAX_CHANNELS,
				   AMQP_DEFAULT_FRAME_SIZE,
				   DEFAULT_HEARTBEAT_SECONDS,
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

	bool declareQueue(const string queueName)
	{
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
				disposeConnection();
				return false;
			}
		}
		return true;
	}

	bool declareConsumerQueue(void)
	{
		if (m_isConsumerQueueDeclared)
			return true;
		m_isConsumerQueueDeclared =
		  declareQueue(getConsumerQueueName());
		return m_isConsumerQueueDeclared;
	}

	bool declarePublisherQueue(void)
	{
		if (m_isPublisherQueueDeclared)
			return true;
		m_isPublisherQueueDeclared =
		  declareQueue(getPublisherQueueName());
		return m_isPublisherQueueDeclared;
	}

	void disposeConnection()
	{
		if (!m_connection)
			return;

		disposeChannel();
		disposeSocket();
		logErrorResponse("destroy connection",
				 amqp_destroy_connection(m_connection));

		m_isConsumerQueueDeclared = false;
		m_isPublisherQueueDeclared = false;
		m_socket = NULL;
		m_connection = NULL;
	}

	time_t getTimeout(void)
	{
		return m_info.getTimeout();
	}

	bool hasConsumerQueueName()
	{
		return !m_info.getConsumerQueueName().empty();
	}

	bool hasPublisherQueueName()
	{
		return !m_info.getPublisherQueueName().empty();
	}

	const string &getConsumerQueueName()
	{
		const string &queueName = m_info.getConsumerQueueName();
		if (queueName.empty())
			MLPL_WARN("The consumerQueueName isn't set!\n");
		return queueName;
	}

	const string &getPublisherQueueName()
	{
		const string &queueName = m_info.getPublisherQueueName();
		if (queueName.empty())
			MLPL_WARN("The publisherQueueName isn't set!\n");
		return queueName;
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
			MLPL_ERR("%s: server exception\n",
				 context);
			switch (reply.reply.id) {
			case AMQP_CHANNEL_CLOSE_METHOD:
			{
				amqp_channel_close_t *result =
				  static_cast<amqp_channel_close_t *>(
				    reply.reply.decoded);
				string message(
				  static_cast<char *>(result->reply_text.bytes),
				  result->reply_text.len);
				MLPL_ERR("Channel close: %s\n",
					 message.c_str());
				break;
			}
			case AMQP_CONNECTION_CLOSE_METHOD:
			{
				amqp_connection_close_t *result =
				  static_cast<amqp_connection_close_t *>(
				    reply.reply.decoded);
				string message(
				  static_cast<char *>(result->reply_text.bytes),
				  result->reply_text.len);
				MLPL_ERR("Connection close: %s\n", message.c_str());
				break;
			}
			default:
				// Other methods won't arrive in this case.
				break;
			}
			break;
		}
		return;
	}

	amqp_channel_t getChannel()
	{
		return m_channel;
	}

	amqp_connection_state_t getConnection()
	{
		return m_connection;
	}

	const AMQPConnectionInfo &getConnectionInfo()
	{
		return m_info;
	}

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
		if (status == AMQP_STATUS_SOCKET_ERROR ||
		    status == AMQP_STATUS_TCP_ERROR) {
			MLPL_ERR("errno: %d: %s\n", errno, strerror(errno));
		}
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

shared_ptr<AMQPConnection> AMQPConnection::create(const AMQPConnectionInfo &info)
{
	return make_shared<AMQPConnection>(info);
}

AMQPConnection::AMQPConnection(const AMQPConnectionInfo &info)
: m_impl(new Impl(info))
{
}

AMQPConnection::~AMQPConnection()
{
}

const AMQPConnectionInfo &AMQPConnection::getConnectionInfo(void)
{
	return m_impl->getConnectionInfo();
}

bool AMQPConnection::connect(void)
{
	m_impl->m_connection = amqp_new_connection();
	if (initializeConnection()) {
		return true;
	} else {
		m_impl->disposeConnection();
		return false;
	}
}

bool AMQPConnection::initializeConnection(void)
{
	if (!m_impl->openSocket())
		return false;
	if (!m_impl->login())
		return false;
	if (!m_impl->openChannel())
		return false;
	return true;
}

time_t AMQPConnection::getTimeout(void)
{
	return m_impl->getTimeout();
}

bool AMQPConnection::isConnected(void)
{
	return m_impl->m_connection;
}

bool AMQPConnection::openSocket(void)
{
	return m_impl->openSocket();
}

bool AMQPConnection::login(void)
{
	return m_impl->login();
}

bool AMQPConnection::openChannel(void)
{
	return m_impl->openChannel();
}

bool AMQPConnection::declareQueue(const string queueName)
{
	return m_impl->declareQueue(queueName);
}

string AMQPConnection::getConsumerQueueName(void)
{
	return m_impl->getConsumerQueueName();
}

string AMQPConnection::getPublisherQueueName(void)
{
	return m_impl->getPublisherQueueName();
}

void AMQPConnection::disposeConnection(void)
{
	return m_impl->disposeConnection();
}

void AMQPConnection::logErrorResponse(const char *context,
	                              const amqp_rpc_reply_t &reply)
{
	return m_impl->logErrorResponse(context, reply);
}

amqp_channel_t AMQPConnection::getChannel(void)
{
	return m_impl->getChannel();
}

amqp_connection_state_t AMQPConnection::getConnection(void)
{
	return m_impl->getConnection();
}

bool AMQPConnection::startConsuming(void)
{
	if (!isConnected())
		return false;
	if (!m_impl->declareConsumerQueue())
		return false;

	const amqp_bytes_t queue =
		amqp_cstring_bytes(getConsumerQueueName().c_str());
	const amqp_bytes_t consumer_tag = amqp_empty_bytes;
	const amqp_boolean_t no_local = false;
	const amqp_boolean_t no_ack = true;
	const amqp_boolean_t exclusive = false;
	const amqp_table_t arguments = amqp_empty_table;
	const amqp_basic_consume_ok_t *response;
	response = amqp_basic_consume(getConnection(),
				      getChannel(),
				      queue,
				      consumer_tag,
				      no_local,
				      no_ack,
				      exclusive,
				      arguments);
	if (!response) {
		const amqp_rpc_reply_t reply =
			amqp_get_rpc_reply(getConnection());
		if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
			logErrorResponse("start consuming", reply);
			disposeConnection();
			return false;
		}
	}
	return true;
}

bool AMQPConnection::consume(AMQPMessage &message)
{
	if (!isConnected())
		return false;
	if (!m_impl->declareConsumerQueue())
		return false;

	amqp_maybe_release_buffers(getConnection());

	struct timeval timeout = {
		getTimeout(),
		0
	};
	const int flags = 0;
	amqp_envelope_t envelope;
	amqp_rpc_reply_t reply = amqp_consume_message(getConnection(),
						      &envelope,
						      &timeout,
						      flags);
	switch (reply.reply_type) {
	case AMQP_RESPONSE_NORMAL:
	{
		const amqp_bytes_t *contentType =
		  &(envelope.message.properties.content_type);
		const amqp_bytes_t *body = &(envelope.message.body);
		message.contentType.assign(
		  static_cast<char*>(contentType->bytes),
		  static_cast<int>(contentType->len));
		message.body.assign(static_cast<char*>(body->bytes),
				    static_cast<int>(body->len));
		amqp_destroy_envelope(&envelope);
		break;
	}
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

	return true;
}

bool AMQPConnection::publish(const AMQPMessage &message)
{
	if (!isConnected())
		return false;
	if (!m_impl->declarePublisherQueue())
		return false;

	const amqp_bytes_t exchange = amqp_empty_bytes;
	const amqp_bytes_t queue =
		amqp_cstring_bytes(getPublisherQueueName().c_str());
	const amqp_boolean_t no_mandatory = false;
	const amqp_boolean_t no_immediate = false;
	int response;
	amqp_basic_properties_t props;
	props._flags =
		AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = message.contentType.empty() ?
		amqp_cstring_bytes("application/octet-stream") :
		amqp_cstring_bytes(message.contentType.c_str());
	props.delivery_mode = 2;
	amqp_bytes_t body_bytes;
	body_bytes.bytes = const_cast<char *>(message.body.data());
	body_bytes.len = message.body.length();
	response = amqp_basic_publish(getConnection(),
				      getChannel(),
				      exchange,
				      queue,
				      no_mandatory,
				      no_immediate,
				      &props,
				      body_bytes);

	if (response != AMQP_STATUS_OK) {
		m_impl->logErrorResponse("publish a message", response);
		m_impl->disposeConnection();
		return false;
	}

	return true;
}

bool AMQPConnection::purgeAllQueues(void)
{
	bool succeeded = true;
	if (!m_impl->hasConsumerQueueName())
		succeeded = purgeQueue(m_impl->getConsumerQueueName());
	if (!m_impl->hasPublisherQueueName())
		succeeded = purgeQueue(m_impl->getPublisherQueueName())
		  && succeeded;
	return succeeded;
}

bool AMQPConnection::purgeQueue(const string queueName)
{
	if (!isConnected())
		return false;

	const amqp_bytes_t queue = amqp_cstring_bytes(queueName.c_str());
	amqp_queue_purge_ok_t *response;
	response = amqp_queue_purge(getConnection(),
				    getChannel(),
				    queue);
	if (!response) {
		const amqp_rpc_reply_t reply =
			amqp_get_rpc_reply(getConnection());
		if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
			logErrorResponse("purge", reply);
			disposeConnection();
			return false;
		}
	}
	return true;
}

bool AMQPConnection::deleteAllQueues(void)
{
	bool succeeded = true;
	if (!m_impl->hasConsumerQueueName())
		succeeded = deleteQueue(m_impl->getConsumerQueueName());
	if (!m_impl->hasPublisherQueueName())
		succeeded = deleteQueue(m_impl->getPublisherQueueName())
		  && succeeded;
	m_impl->m_isConsumerQueueDeclared = false;
	m_impl->m_isPublisherQueueDeclared = false;
	return succeeded;
}

bool AMQPConnection::deleteQueue(const string queueName)
{
	if (!isConnected())
		return false;

	const amqp_bytes_t queue = amqp_cstring_bytes(queueName.c_str());
	amqp_queue_delete_ok_t *response;
	response = amqp_queue_delete(getConnection(),
				     getChannel(),
				     queue,
				     false,  // delete using queue
				     false); // delete non empty queue
	if (!response) {
		const amqp_rpc_reply_t reply =
			amqp_get_rpc_reply(getConnection());
		if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
			logErrorResponse("purge", reply);
			disposeConnection();
			return false;
		}
	}
	return true;
}
