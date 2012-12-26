#include "utils.h"
#include "face-mysql-worker.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
face_mysql_worker::face_mysql_worker(GSocket *sock)
: m_socket(sock)
{
}

face_mysql_worker::~face_mysql_worker()
{
	if (m_socket)
		g_object_unref(m_socket);
}

void face_mysql_worker::start(void)
{
	ASURA_P(DBG, "%s\n", __PRETTY_FUNCTION__);
}

