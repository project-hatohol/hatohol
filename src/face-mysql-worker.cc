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
	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
gpointer face_mysql_worker::_main_thread(gpointer data)
{
	face_mysql_worker *obj = static_cast<face_mysql_worker *>(data);
	return obj->main_thread();
}
