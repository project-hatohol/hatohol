#include "utils.h"
#include "face-mysql-worker.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
face_mysql_worker::face_mysql_worker(GSocket *sock)
: m_thread(NULL),
  m_socket(sock)
{
}

face_mysql_worker::~face_mysql_worker()
{
	if (m_socket)
		g_object_unref(m_socket);
	if (m_thread)
		g_thread_unref(m_thread);
}

void face_mysql_worker::start(void)
{
	ASURA_P(DBG, "%s\n", __PRETTY_FUNCTION__);
	GError *error = NULL;
	m_thread = g_thread_try_new("face-mysql-worker", _main_thread, this,
	                            &error);
	if (m_thread == NULL) {
		ASURA_P(ERR, "Failed to call g_thread_try_new: %s\n",
		        error->message);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer face_mysql_worker::main_thread(void)
{
	ASURA_P(DBG, "%s\n", __PRETTY_FUNCTION__);

	// send handshake
	uint8_buffer buf;
	make_handshake_v10(buf);
	if (!send(buf))
		return NULL;

	return NULL;
}

void face_mysql_worker::make_handshake_v10(uint8_buffer &buf)
{
}

bool face_mysql_worker::send(uint8_buffer &buf)
{
	GError *error = NULL;
	gssize ret = g_socket_send(m_socket, buf, buf.size(),
	                           NULL, &error);
	if (ret == -1) {
		ASURA_P(ERR, "Failed to call g_socket_send: %s\n",
		        error->message);
		g_error_free(error);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
gpointer face_mysql_worker::_main_thread(gpointer data)
{
	face_mysql_worker *obj = static_cast<face_mysql_worker *>(data);
	return obj->main_thread();
}

