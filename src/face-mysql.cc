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
face_mysql::face_mysql(void)
: m_socket(NULL)
{
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
	sockaddr.sin_port = htons(DEFAULT_PORT);
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
		face_mysql_worker *worker = new face_mysql_worker(sock);
		worker->start();
	}

	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
