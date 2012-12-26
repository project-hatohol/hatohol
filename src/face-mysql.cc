#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "utils.h"
#include "face-mysql.h"
#include "face-mysql-worker.h"

static const int DEFAULT_PORT = 3306;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
face_mysql::face_mysql(command_line_arg_t &cmd_arg)
: m_socket(NULL),
  m_port(DEFAULT_PORT)
{
	for (size_t i = 0; i < cmd_arg.size(); i++) {
		string &cmd = cmd_arg[i];
		if (cmd == "--face-mysql-port")
			i = parse_cmd_arg_port(cmd_arg, i);
	}
	ASURA_P(INFO, "started face-mysql, port: %d\n", m_port);
}

face_mysql::~face_mysql()
{
	if (m_socket)
		g_object_unref(m_socket);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer face_mysql::main_thread(face_thread_arg_t *arg)
{
	GError *error = NULL;
	m_socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
	                        G_SOCKET_PROTOCOL_TCP, &error);
	if (m_socket == NULL) {
		ASURA_P(ERR, "Failed to create thread: %s\n", error->message);
		g_error_free(error);
		return NULL;
	}

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(m_port);
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	GSocketAddress *address =
	  g_socket_address_new_from_native(&sockaddr, sizeof(sockaddr));
	if (!address) {
		ASURA_P(ERR,
		        "Failed to call g_socket_address_new_from_native: %s\n",
		        error->message);
		g_error_free(error);
		return NULL;
	}

	if (!g_socket_bind(m_socket, address, TRUE, &error)) {
		ASURA_P(ERR, "Failed to call g_socket_bind: %s\n",
		        error->message);
		g_error_free(error);
		return NULL;
	}

	if (!g_socket_listen(m_socket, &error)) {
		ASURA_P(ERR, "Failed to call g_socket_listen: %s\n",
		        error->message);
		g_error_free(error);
		return NULL;
	}

	uint32_t conn_id = 0;
	while (true) {
		// GSocket obtained here have to be freed in
		// the face_mysql_worker instance.
		GSocket *sock = g_socket_accept(m_socket, NULL, &error);
		if (sock == NULL) {
			ASURA_P(ERR, "Failed to call g_socket_accept: %s\n",
			        error->message);
			g_error_free(error);
			error = NULL;
			continue;
		}
		face_mysql_worker *worker =
		  new face_mysql_worker(sock, conn_id);
		worker->start();
		conn_id++;
	}

	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
size_t face_mysql::parse_cmd_arg_port(command_line_arg_t &cmd_arg, size_t idx)
{
	if (idx == cmd_arg.size() - 1) {
		ASURA_P(ERR, "needs port number.");
		return idx;
	}

	idx++;
	string &port_str = cmd_arg[idx];
	int port = atoi(port_str.c_str());
	if (port < 0 || port > 65536) {
		ASURA_P(ERR, "invalid port: %s, %d\n", port_str.c_str(), port);
		return idx;
	}

	m_port = port;
	return idx;
}
