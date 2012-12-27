#include <cstring>

#include <Logger.h>
using namespace mlpl;

#include "Utils.h"
#include "FaceMySQLWorker.h"

static const int SERVER_STATUS_AUTOCOMMIT = 0x02;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FaceMySQLWorker::FaceMySQLWorker(GSocket *sock, uint32_t connId)
: m_thread(NULL),
  m_socket(sock),
  m_connId(connId)
{
}

FaceMySQLWorker::~FaceMySQLWorker()
{
	if (m_socket)
		g_object_unref(m_socket);
	if (m_thread)
		g_thread_unref(m_thread);
}

void FaceMySQLWorker::start(void)
{
	MLPL_DBG("%s\n", __PRETTY_FUNCTION__);
	GError *error = NULL;
	m_thread = g_thread_try_new("face-mysql-worker", _mainThread, this,
	                            &error);
	if (m_thread == NULL) {
		MLPL_ERR("Failed to call g_thread_try_new: %s\n",
		         error->message);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer FaceMySQLWorker::mainThread(void)
{
	MLPL_DBG("%s\n", __PRETTY_FUNCTION__);

	// send handshake
	SmartBuffer buf;
	makeHandshakeV10(buf);
	if (!send(buf))
		return NULL;

	return NULL;
}

void FaceMySQLWorker::makeHandshakeV10(SmartBuffer &buf)
{
	// protocol version
	static const char SERVER_VERSION[] = "5.5.10";
	char authPluginData[] = "AUTH_PLUGIN_DATA";

	static const int LEN_PKT_LENGTH = 4;
	static const int LEN_PROTOCOL_VERSION = 1;
	static const int LEN_SERVER_VERSION = sizeof(SERVER_VERSION);
	static const int LEN_CONNECTION_ID = 4;
	static const int LEN_AUTH_PLUGIN_DATA_PART_1 = 8;
	static const int LEN_FILLER_1 = 1;
	static const int LEN_CAPABILITY_FLAGS_1 = 2;
	static const int LEN_CARACTER_SET = 1;
	static const int LEN_STATUS_FLAGS = 2;
	static const int LEN_CAPABILITY_FLAGS_2 = 2;
	static const int LEN_LENGTH_AUTH_PLUGIN_DATA = 1;
	static const int LEN_RESERVED = 10;

	static const uint32_t LEN_TOTAL =
	  LEN_PKT_LENGTH + LEN_PROTOCOL_VERSION +
	  LEN_SERVER_VERSION + LEN_CONNECTION_ID +
	  LEN_AUTH_PLUGIN_DATA_PART_1 + LEN_FILLER_1 +
	  LEN_CAPABILITY_FLAGS_1 + LEN_CARACTER_SET + LEN_STATUS_FLAGS +
	  LEN_CAPABILITY_FLAGS_2 + LEN_LENGTH_AUTH_PLUGIN_DATA + LEN_RESERVED;

	static const uint8_t PROTOCOL_VERSION = 10;

	buf.alloc(LEN_TOTAL);
	buf.add32(LEN_TOTAL);
	buf.add8(PROTOCOL_VERSION);
	buf.add(SERVER_VERSION, LEN_SERVER_VERSION);
	buf.add32(m_connId);
	buf.add(authPluginData, LEN_AUTH_PLUGIN_DATA_PART_1);
	buf.add8(0); // Filler

	static const uint16_t capability1 = 0;
	buf.add16(capability1);

	static const uint8_t char_set = 8; // latin1?
	buf.add8(char_set);

	static const uint8_t status = SERVER_STATUS_AUTOCOMMIT;
	buf.add16(status);

	static const uint16_t capability2 = 0;
	buf.add16(capability2);

	uint8_t lenAuthPluginData = strlen(authPluginData);
	buf.add8(lenAuthPluginData);

	buf.addZero(LEN_RESERVED);

	buf.add(&authPluginData[LEN_AUTH_PLUGIN_DATA_PART_1],
	        lenAuthPluginData - LEN_AUTH_PLUGIN_DATA_PART_1 + 1);
}

bool FaceMySQLWorker::send(SmartBuffer &buf)
{
	GError *error = NULL;
	gssize ret = g_socket_send(m_socket, buf, buf.size(),
	                           NULL, &error);
	if (ret == -1) {
		MLPL_ERR("Failed to call g_socket_send: %s\n",
		        error->message);
		g_error_free(error);
		return false;
	}

	if (ret != (gssize)buf.size()) {
		MLPL_ERR("ret: %zd != buf.size(); %zd\n", ret, buf.size());
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
gpointer FaceMySQLWorker::_mainThread(gpointer data)
{
	FaceMySQLWorker *obj = static_cast<FaceMySQLWorker *>(data);
	return obj->mainThread();
}

