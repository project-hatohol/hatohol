#ifndef face_mysql_h
#define face_mysql_h

#include <gio/gio.h>
#include "face-base.h"

class face_mysql : public face_base {
	GSocket *m_socket;

protected:
	// virtual methods
	gpointer main_thread(face_thread_arg_t *arg);

public:
	face_mysql(void);
	virtual ~face_mysql();
};

#endif // face_mysql_h
