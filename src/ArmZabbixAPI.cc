#include <Logger.h>
using namespace mlpl;

#include <libsoup/soup.h>
#include "ArmZabbixAPI.h"

static const int DEFAULT_SERVER_PORT = 80;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmZabbixAPI::ArmZabbixAPI(CommandLineArg &cmdArg)
: m_server_port(DEFAULT_SERVER_PORT)
{
	m_server = "localhost";
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer ArmZabbixAPI::mainThread(AsuraThreadArg *arg)
{
	MLPL_INFO("started: %s\n", __PRETTY_FUNCTION__);
	
	SoupSession *session = soup_session_sync_new();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET,
	                                    "http://localhost/");
	guint ret = soup_session_send_message(session, msg);
	MLPL_INFO("[%s]: ret: %d\n", __PRETTY_FUNCTION__, ret);

	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
