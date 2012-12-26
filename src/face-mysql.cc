#include <glib.h>

#include "utils.h"
#include "face-mysql.h"

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

	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
