#include <cstring>
using namespace std;

#include <Logger.h>
using namespace mlpl;

#include "Utils.h"
#include "FaceMySQLWorker.h"

static const uint32_t PACKET_ID_MASK   = 0xff000000;
static const uint32_t PACKET_SIZE_MASK = 0x00ffffff;
static const int PACKET_ID_SHIFT_BITS = 24;

static const int HANDSHAKE_RESPONSE41_RESERVED_SIZE = 23;

static const uint32_t SERVER_STATUS_AUTOCOMMIT = 0x00000002;

static const uint32_t CLIENT_LONG_PASSWORD     = 0x00000001;
static const uint32_t CLIENT_CLIENT_FOUND_ROWS = 0x00000002;
static const uint32_t CLIENT_CLIENT_LONG_FLAG  = 0x00000004;
static const uint32_t CLIENT_CONNECT_WITH_DB   = 0x00000008;
static const uint32_t CLIENT_NO_SCHEMA         = 0x00000010;
static const uint32_t CLIENT_COMPRESS          = 0x00000020;
static const uint32_t CLIENT_ODBC              = 0x00000040;
static const uint32_t CLIENT_LOCAL_FILES       = 0x00000080;
static const uint32_t CLIENT_IGNORE_SPACE      = 0x00000100;
static const uint32_t CLIENT_PROTOCOL_41       = 0x00000200;
static const uint32_t CLIENT_INTERACTIVE       = 0x00000400;
static const uint32_t CLIENT_SSL               = 0x00000800;
static const uint32_t CLIENT_IGNORE_SIGPIPE    = 0x00001000;
static const uint32_t CLIENT_TRANSACTIONS      = 0x00002000;
static const uint32_t CLIENT_RESERVED          = 0x00004000;
static const uint32_t CLIENT_SECURE_CONNECTION = 0x00008000;
static const uint32_t CLIENT_MULTI_STATEMENTS  = 0x00010000;
static const uint32_t CLIENT_MULTI_RESULTS     = 0x00020000;
static const uint32_t CLIENT_PS_MULTI_RESULTS  = 0x00040000;
static const uint32_t CLIENT_PLUGIN_AUTH       = 0x00080000;
static const uint32_t CLIENT_CONNECT_ATTRS     = 0x00100000;
static const uint32_t CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA = 0x00200000;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FaceMySQLWorker::FaceMySQLWorker(GSocket *sock, uint32_t connId)
: m_thread(NULL),
  m_socket(sock),
  m_connId(connId),
  m_packetId(0)
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
	if (!receiveHandshakeResponse41())
		return NULL;

	return NULL;
}

void FaceMySQLWorker::makeHandshakeV10(SmartBuffer &buf)
{
	// protocol version
	static const char SERVER_VERSION[] = "5.5.10";
	static const char AUTH_PLUGIN_NAME[] = "mysql_native_password";
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
	static const int LEN_AUTH_PLUGIN_NAME = sizeof(AUTH_PLUGIN_NAME);

	const uint8_t lenAuthPluginData = strlen(authPluginData);
	const uint32_t lenAuthPluginDataPart2 = 
	        lenAuthPluginData - LEN_AUTH_PLUGIN_DATA_PART_1 + 1;
	const uint32_t LEN_TOTAL =
	  LEN_PKT_LENGTH + LEN_PROTOCOL_VERSION +
	  LEN_SERVER_VERSION + LEN_CONNECTION_ID +
	  LEN_AUTH_PLUGIN_DATA_PART_1 + LEN_FILLER_1 +
	  LEN_CAPABILITY_FLAGS_1 + LEN_CARACTER_SET + LEN_STATUS_FLAGS +
	  LEN_CAPABILITY_FLAGS_2 + LEN_LENGTH_AUTH_PLUGIN_DATA + LEN_RESERVED +
	  lenAuthPluginDataPart2 + 
	  LEN_AUTH_PLUGIN_NAME;

	buf.alloc(LEN_TOTAL);
	buf.add32(LEN_TOTAL - LEN_PKT_LENGTH);

	static const uint8_t PROTOCOL_VERSION = 10;
	buf.add8(PROTOCOL_VERSION);

	buf.add(SERVER_VERSION, LEN_SERVER_VERSION);
	buf.add32(m_connId);
	buf.add(authPluginData, LEN_AUTH_PLUGIN_DATA_PART_1);
	buf.add8(0); // Filler

	static const uint16_t capability1 =
	  CLIENT_LONG_PASSWORD |
	  CLIENT_CLIENT_FOUND_ROWS |
	  CLIENT_CLIENT_LONG_FLAG |
	  CLIENT_CONNECT_WITH_DB |
	  CLIENT_NO_SCHEMA |
	  CLIENT_COMPRESS |
	  CLIENT_ODBC |
	  CLIENT_LOCAL_FILES |
	  CLIENT_IGNORE_SPACE |
	  CLIENT_PROTOCOL_41 |
	  CLIENT_INTERACTIVE  |
	  // CLIENT_SSL
	  CLIENT_IGNORE_SIGPIPE |
	  CLIENT_TRANSACTIONS |
	  CLIENT_RESERVED |
	  CLIENT_SECURE_CONNECTION;
	buf.add16(capability1);

	static const uint8_t char_set = 8; // latin1?
	buf.add8(char_set);

	static const uint8_t status = SERVER_STATUS_AUTOCOMMIT;
	buf.add16(status);

	static const uint16_t capability2 = (
	  CLIENT_MULTI_STATEMENTS |
	  CLIENT_MULTI_RESULTS |
	  CLIENT_PS_MULTI_RESULTS |
	  CLIENT_PLUGIN_AUTH
	  // CLIENT_CONNECT_ATTRS
	  // CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA
	) >> 16;

	buf.add16(capability2);

	buf.add8(lenAuthPluginData);

	buf.addZero(LEN_RESERVED);

	buf.add(&authPluginData[LEN_AUTH_PLUGIN_DATA_PART_1],
	        lenAuthPluginDataPart2);
	buf.add(AUTH_PLUGIN_NAME, LEN_AUTH_PLUGIN_NAME);
}

bool FaceMySQLWorker::receiveHandshakeResponse41(void)
{
	uint32_t pktHeader;
	if (!recive(reinterpret_cast<char *>(&pktHeader), sizeof(pktHeader)))
		return false;
	m_packetId = (pktHeader & PACKET_ID_MASK) >> PACKET_ID_SHIFT_BITS;
	uint32_t pktSize = pktHeader & PACKET_SIZE_MASK;
	MLPL_INFO("pktSize: %08x (%d)\n", pktSize, m_packetId);

	// read response body
	SmartBuffer buf(pktSize);
	if (!recive(buf, pktSize))
		return false;

	buf.resetIndex();
	uint16_t *capabilityLsb = buf.getCurrPointer<uint16_t>();
	if (!((*capabilityLsb) & CLIENT_PROTOCOL_41)) {
		MLPL_CRIT("Currently client protocol other than 4.1 has "
		          "not been implented.\n");
		return false;
	}

	// fill the structure
	memset(&m_hsResp41, 0, sizeof(m_hsResp41));
	m_hsResp41.capability    = *buf.getCurrPointerAndIncIndex<uint32_t>();
	m_hsResp41.maxPacketSize = *buf.getCurrPointerAndIncIndex<uint32_t>();
	m_hsResp41.characterSet  = *buf.getCurrPointerAndIncIndex<uint8_t>();
	buf.incIndex(HANDSHAKE_RESPONSE41_RESERVED_SIZE);
	m_hsResp41.username = buf.getCurrPointer<char>();
	buf.incIndex(m_hsResp41.username.size() + 1);
	MLPL_INFO("CAP: %08x, username: %s\n", m_hsResp41.capability,
	          m_hsResp41.username.c_str());
	if (m_hsResp41.capability & CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) {
	} else if (m_hsResp41.capability & CLIENT_SECURE_CONNECTION) {
		uint8_t size = *buf.getCurrPointerAndIncIndex<uint8_t>();
		m_hsResp41.lenAuthResponse = size;
		m_hsResp41.authResponse = string(buf.getCurrPointer<char *>(),
		                                 0, size);
	} else {
		m_hsResp41.authResponse = buf.getCurrPointer<char *>();
	}

	return true;
}

bool FaceMySQLWorker::recive(char* buf, size_t size)
{
	GError *error = NULL;

	int remain_size = size;
	while (remain_size > 0) {
		gssize ret = g_socket_receive(m_socket, buf, remain_size,
		                              NULL, &error);
		if (ret == -1) {
			MLPL_ERR("Failed to call g_socket_receive: %s\n",
			        error->message);
			g_error_free(error);
			return false;
		}
		if (ret == 0)
			return false;
		if (ret > remain_size) {
			MLPL_BUG("ret: %d > remain_size: %zd\n",
			         ret, remain_size);
			return false;
		}
		remain_size -= ret;
	}
	return true;
}

bool FaceMySQLWorker::send(SmartBuffer &buf)
{
	GError *error = NULL;
	gssize ret = g_socket_send(m_socket, buf, buf.index(), NULL, &error);
	if (ret == -1) {
		MLPL_ERR("Failed to call g_socket_send: %s\n",
		        error->message);
		g_error_free(error);
		return false;
	}

	if (ret != (gssize)buf.index()) {
		MLPL_ERR("ret: %zd != buf.index(); %zd\n", ret, buf.index());
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

