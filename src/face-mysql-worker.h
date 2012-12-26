#ifndef face_mysql_worker_h
#define face_mysql_worker_h

#include <glib.h>
#include <gio/gio.h>

#include "smart-buffer.h"
#include "utils.h"

class face_mysql_worker {
	GThread *m_thread;
	GSocket *m_socket;

	static gpointer _main_thread(gpointer data);
protected:
	gpointer main_thread(void);
	void make_handshake_v10(smart_buffer &buf);
	bool send(smart_buffer &buf);

public:
	face_mysql_worker(GSocket *sock);
	virtual ~face_mysql_worker();
	void start(void);
};

#endif // face_mysql_worker_h
