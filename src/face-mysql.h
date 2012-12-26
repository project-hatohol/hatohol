#ifndef face_mysql_h
#define face_mysql_h

#include <vector>
#include <string>
using namespace std;

#include <gio/gio.h>
#include "utils.h"
#include "face-base.h"

class face_mysql : public face_base {
	GSocket *m_socket;
	int      m_port;

	size_t parse_cmd_arg_port(command_line_arg_t &cmd_arg, size_t idx);
protected:
	// virtual methods
	gpointer main_thread(face_thread_arg_t *arg);

public:
	face_mysql(command_line_arg_t &cmd_arg);
	virtual ~face_mysql();
};

#endif // face_mysql_h
