#include <cstring>
#include "utils.h"
#include "face-mysql-worker.h"

static const int SERVER_STATUS_AUTOCOMMIT = 0x02;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
face_mysql_worker::face_mysql_worker(GSocket *sock, uint32_t conn_id)
: m_thread(NULL),
  m_socket(sock),
  m_conn_id(conn_id)
{
}

face_mysql_worker::~face_mysql_worker()
{
	if (m_socket)
		g_object_unref(m_socket);
	if (m_thread)
		g_thread_unref(m_thread);
}

void face_mysql_worker::start(void)
{
	ASURA_P(DBG, "%s\n", __PRETTY_FUNCTION__);
	GError *error = NULL;
	m_thread = g_thread_try_new("face-mysql-worker", _main_thread, this,
	                            &error);
	if (m_thread == NULL) {
		ASURA_P(ERR, "Failed to call g_thread_try_new: %s\n",
		        error->message);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer face_mysql_worker::main_thread(void)
{
	ASURA_P(DBG, "%s\n", __PRETTY_FUNCTION__);

	// send handshake
	smart_buffer buf;
	make_handshake_v10(buf);
	if (!send(buf))
		return NULL;

	return NULL;
}

void face_mysql_worker::make_handshake_v10(smart_buffer &buf)
{
	// protocol version
	static const char SERVER_VERSION[] = "5.5.10";
	char auth_plugin_data[] = "AUTH_PLUGIN_DATA";

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
	buf.add32(m_conn_id);
	buf.add(auth_plugin_data, LEN_AUTH_PLUGIN_DATA_PART_1);
	buf.add8(0); // Filler

	static const uint16_t capability1 = 0;
	buf.add16(capability1);

	static const uint8_t char_set = 8; // latin1?
	buf.add8(char_set);

	static const uint8_t status = SERVER_STATUS_AUTOCOMMIT;
	buf.add16(status);

	static const uint16_t capability2 = 0;
	buf.add16(capability2);

	uint8_t len_auth_plugin_data = strlen(auth_plugin_data);
	buf.add8(len_auth_plugin_data);

	buf.add_zero(LEN_RESERVED);
}

bool face_mysql_worker::send(smart_buffer &buf)
{
	GError *error = NULL;
	gssize ret = g_socket_send(m_socket, buf, buf.size(),
	                           NULL, &error);
	if (ret == -1) {
		ASURA_P(ERR, "Failed to call g_socket_send: %s\n",
		        error->message);
		g_error_free(error);
		return false;
	}

	if (ret != (gssize)buf.size()) {
		ASURA_P(ERR, "ret: %zd != buf.size(); %zd\n", ret, buf.size());
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
gpointer face_mysql_worker::_main_thread(gpointer data)
{
	face_mysql_worker *obj = static_cast<face_mysql_worker *>(data);
	return obj->main_thread();
}

