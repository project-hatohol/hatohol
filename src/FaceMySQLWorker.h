#ifndef FaceMySQLWorker_h
#define FaceMySQLWorker_h

#include <glib.h>
#include <gio/gio.h>

#include <SmartBuffer.h>
using namespace mlpl;

#include "Utils.h"

class FaceMySQLWorker {
	GThread *m_thread;
	GSocket *m_socket;
	uint32_t m_connId;

	static gpointer _mainThread(gpointer data);
protected:
	gpointer mainThread(void);
	void makeHandshakeV10(SmartBuffer &buf);
	bool send(SmartBuffer &buf);

public:
	FaceMySQLWorker(GSocket *sock, uint32_t connId);
	virtual ~FaceMySQLWorker();
	void start(void);
};

#endif // FaceMySQLWorker_h
