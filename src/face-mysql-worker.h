#ifndef face_mysql_worker_h
#define face_mysql_worker_h

#include <glib.h>
#include <gio/gio.h>

class face_mysql_worker {
	GThread *m_thread;
	GSocket *m_socket;

	static gpointer _main_thread(gpointer data);

protected:
	gpointer main_thread(void);

public:
	face_mysql_worker(GSocket *sock);
	virtual ~face_mysql_worker();
	void start(void);
};

#endif // face_mysql_worker_h
