#ifndef face_mysql_worker_h
#define face_mysql_worker_h

#include <gio/gio.h>

class face_mysql_worker {
	GSocket *m_socket;
public:
	face_mysql_worker(GSocket *sock);
	virtual ~face_mysql_worker();
	void start(void);
};

#endif // face_mysql_worker_h
