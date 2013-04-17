#ifndef ZabbixAPIEmulator_h
#define ZabbixAPIEmulator_h

#include <glib.h>
#include <libsoup/soup.h>
#include "JsonParserAgent.h"

enum OperationMode {
	OPE_MODE_NORMAL,
	OPE_MODE_HTTP_NOT_FOUND,
};

class ZabbixAPIEmulator {
public:
	struct APIHandlerArg;
	typedef void (ZabbixAPIEmulator::*APIHandler)(APIHandlerArg &);

	ZabbixAPIEmulator(void);
	virtual ~ZabbixAPIEmulator();

	bool isRunning(void);
	void start(guint port);
	void setOperationMode(OperationMode mode);

protected:
	static gpointer _mainThread(gpointer data);
	gpointer mainThread(void);
	static void startObject(JsonParserAgent &parser, const string &name);
	static void handlerDefault
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);
	static void handlerAPI
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);

	string generateAuthToken(void);
	void handlerAPIDispatch(APIHandlerArg &arg);
	void APIHandlerUserLogin(APIHandlerArg &arg);
	void APIHandlerTriggerGet(APIHandlerArg &arg);
	void APIHandlerItemGet(APIHandlerArg &arg);
	void APIHandlerHostGet(APIHandlerArg &arg);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ZabbixAPIEmulator_h
