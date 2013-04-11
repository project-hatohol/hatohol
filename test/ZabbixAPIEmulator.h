#ifndef ZabbixAPIEmulator_h
#define ZabbixAPIEmulator_h

#include <glib.h>
#include <libsoup/soup.h>

enum OperationMode {
	OPE_MODE_NORMAL,
	OPE_MODE_HTTP_NOT_FOUND,
};

class ZabbixAPIEmulator {
public:
	ZabbixAPIEmulator(void);
	virtual ~ZabbixAPIEmulator();

	bool isRunning(void);
	void start(guint port);
	void setOperationMode(OperationMode mode);

protected:
	static gpointer _mainThread(gpointer data);
	gpointer mainThread(void);
	static void handlerDefault
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);
	static void handlerAPI
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ZabbixAPIEmulator_h
