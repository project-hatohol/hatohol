#include <cstring>
#include <algorithm>
using namespace std;

#include <Logger.h>
#include <StringUtils.h>
#include <ParsableString.h>
using namespace mlpl;

#include "Utils.h"
#include "FaceMySQLWorker.h"
#include "SQLProcessorFactory.h"

static const int PACKET_HEADER_SIZE = 4;
static const int MAX_LENENC_INT_LENGTH = 9;
static const uint32_t PACKET_ID_MASK   = 0xff000000;
static const uint32_t PACKET_SIZE_MASK = 0x00ffffff;
static const int PACKET_ID_SHIFT_BITS = 24;

static const int HANDSHAKE_RESPONSE41_RESERVED_SIZE = 23;

enum {
	SERVER_STATUS_IN_TRANS             = 0x0001,
	SERVER_STATUS_AUTOCOMMIT           = 0x0002,
	SERVER_MORE_RESULTS_EXISTS         = 0x0008,
	SERVER_STATUS_NO_GOOD_INDEX_USED   = 0x0010,
	SERVER_STATUS_NO_INDEX_USED        = 0x0020,
	SERVER_STATUS_CURSOR_EXISTS        = 0x0040,
	SERVER_STATUS_LAST_ROW_SENT        = 0x0080,
	SERVER_STATUS_DB_DROPPED           = 0x0100,
	SERVER_STATUS_NO_BACKSLASH_ESCAPES = 0x0200,
	SERVER_STATUS_METADATA_CHANGED     = 0x0400,
	SERVER_QUERY_WAS_SLOW              = 0x0800,
	SERVER_PS_OUT_PARAMS               = 0x1000,
};

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

enum {
	OK_HEADER = 0x00,
	EOF_HEADER = 0xfe,
};

enum {
	ID_COM_QUIT       = 0x01,
	ID_COM_INIT_DB    = 0x02,
	ID_COM_QUERY      = 0x03,
	ID_COM_SET_OPTION = 0x1b,
};

enum {
	TYPE_VAR_LONG   = 0x03,
	TYPE_VAR_STRING = 0xfd,
	TYPE_VAR_UNKNOWN = 0xffff,
};

static const uint8_t RESULT_SET_ROW_NULL = 0xfb;

enum {
	MYSQL_OPTION_MULTI_STATEMENTS_ON  = 0x00,
	MYSQL_OPTION_MULTI_STATEMENTS_OFF = 0x01,
};

int FaceMySQLWorker::m_typeConverTable[TYPE_CONVERT_TABLE_SIZE];
map<string, FaceMySQLWorker::SelectProcFunc>
  FaceMySQLWorker::m_selectProcFuncMap;

static const int DEFAULT_CHAR_SET = 8;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
void FaceMySQLWorker::init(void)
{
	for (size_t i = 0; i < SQL_COLUMN_TYPE_INT; i++)
		m_typeConverTable[i] = TYPE_VAR_UNKNOWN;
	m_typeConverTable[SQL_COLUMN_TYPE_INT] = TYPE_VAR_LONG;
	m_typeConverTable[SQL_COLUMN_TYPE_VARCHAR] = TYPE_VAR_STRING;

	m_selectProcFuncMap["@@version_comment"] =
	  &FaceMySQLWorker::querySelectVersionComment;
	m_selectProcFuncMap["DATABASE()"] =
	  &FaceMySQLWorker::querySelectDatabase;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FaceMySQLWorker::FaceMySQLWorker(GSocket *sock, uint32_t connId)
: m_socket(sock),
  m_connId(connId),
  m_packetId(-1),
  m_charSet(DEFAULT_CHAR_SET),
  m_statusFlags(SERVER_STATUS_AUTOCOMMIT),
  m_sqlProcessor(NULL),
  m_separatorSpaceComma(" ,")
{
	initHandshakeResponse41(m_hsResp41);

	m_cmdProcMap[ID_COM_QUIT]    = &FaceMySQLWorker::comQuit;
	m_cmdProcMap[ID_COM_INIT_DB] = &FaceMySQLWorker::comInitDB;
	m_cmdProcMap[ID_COM_QUERY]   = &FaceMySQLWorker::comQuery;
	m_cmdProcMap[ID_COM_SET_OPTION]  = &FaceMySQLWorker::comSetOption;

	m_queryProcMap["select"]  = &FaceMySQLWorker::querySelect;
	m_queryProcMap["set"]     = &FaceMySQLWorker::querySet;
}

FaceMySQLWorker::~FaceMySQLWorker()
{
	if (m_socket)
		g_object_unref(m_socket);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer FaceMySQLWorker::mainThread(AsuraThreadArg *arg)
{
	MLPL_DBG("%s\n", __PRETTY_FUNCTION__);

	// send handshake
	if (!sendHandshakeV10())
		return NULL;
	if (!receiveHandshakeResponse41())
		return NULL;
	if (!sendOK())
		return NULL;

	while (true) {
		if (!receiveRequest())
			break;
	}

	return NULL;
}

uint32_t FaceMySQLWorker::makePacketHeader(uint32_t length)
{
	uint32_t header = length & PACKET_SIZE_MASK;
	header |= (m_packetId << PACKET_ID_SHIFT_BITS);
	return header;
}

void FaceMySQLWorker::addPacketHeaderRegion(SmartBuffer &pkt)
{
	pkt.add32(0);
}

void FaceMySQLWorker::addLenEncInt(SmartBuffer &buf, uint64_t num)
{
	// calculate size
	size_t size = 0;
	if (num < 0xfc)
		size = 1;
	else if (num < 0x10000)
		size = 3;
	else if (num < 0x1000000)
		size = 4;
	else
		size = 9;
	buf.ensureRemainingSize(size);

	// fill content
	if (num < 0xfc) {
		buf.add8(num);
	} else if (num < 0x10000) {
		buf.add8(0xfc);
		buf.add16(num);
	} else if (num < 0x1000000) {
		buf.add8(0xfd);
		uint8_t numMsb8 =  (num & 0xff0000) >> 16;
		uint16_t numLsb16 =  num & 0xffff;
		buf.add16(numLsb16);
		buf.add8(numMsb8);
	} else {
		buf.add8(0xfe);
		buf.add64(num);
	}
}

void FaceMySQLWorker::allocAndAddPacketHeaderRegion(SmartBuffer &pkt,
                                                    size_t packetSize)
{
	pkt.alloc(packetSize);
	addPacketHeaderRegion(pkt);
}

void FaceMySQLWorker::addLenEncStr(SmartBuffer &pkt, string &str)
{
	addLenEncInt(pkt, str.size());
	pkt.addEx(str.c_str(), str.size());
}

bool FaceMySQLWorker::sendHandshakeV10(void)
{
	// protocol version
	static const char SERVER_VERSION[] = "5.5.10";
	static const char AUTH_PLUGIN_NAME[] = "mysql_native_password";

	// TODO: implement correctly
	char authPluginData[] = {0x66, 0x60, 0x2d, 0x5f, 0x78, 0x60, 0x54,
	                         0x67, 0x28, 0x30, 0x55, 0x3e, 0x73, 0x4b,
	                         0x5c, 0x6c, 0x2b, 0x2e, 0x46, 0x54, 0x00};

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
	  PACKET_HEADER_SIZE + LEN_PROTOCOL_VERSION +
	  LEN_SERVER_VERSION + LEN_CONNECTION_ID +
	  LEN_AUTH_PLUGIN_DATA_PART_1 + LEN_FILLER_1 +
	  LEN_CAPABILITY_FLAGS_1 + LEN_CARACTER_SET + LEN_STATUS_FLAGS +
	  LEN_CAPABILITY_FLAGS_2 + LEN_LENGTH_AUTH_PLUGIN_DATA + LEN_RESERVED +
	  lenAuthPluginDataPart2 +
	  LEN_AUTH_PLUGIN_NAME;

	SmartBuffer pkt(LEN_TOTAL);
	addPacketHeaderRegion(pkt);

	static const uint8_t PROTOCOL_VERSION = 10;
	pkt.add8(PROTOCOL_VERSION);

	pkt.add(SERVER_VERSION, LEN_SERVER_VERSION);
	pkt.add32(m_connId);
	pkt.add(authPluginData, LEN_AUTH_PLUGIN_DATA_PART_1);
	pkt.add8(0); // Filler

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
	pkt.add16(capability1);
	pkt.add8(m_charSet);
	pkt.add16(m_statusFlags);

	static const uint16_t capability2 = (
	  CLIENT_MULTI_STATEMENTS |
	  CLIENT_MULTI_RESULTS |
	  CLIENT_PS_MULTI_RESULTS |
	  CLIENT_PLUGIN_AUTH
	  // CLIENT_CONNECT_ATTRS
	  // CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA
	) >> 16;

	pkt.add16(capability2);
	pkt.add8(lenAuthPluginData + 1); // +1 is for NULL Term.
	pkt.addZero(LEN_RESERVED);
	pkt.add(&authPluginData[LEN_AUTH_PLUGIN_DATA_PART_1],
	        lenAuthPluginDataPart2);
	pkt.add(AUTH_PLUGIN_NAME, LEN_AUTH_PLUGIN_NAME);

	return sendPacket(pkt);
}

bool FaceMySQLWorker::receiveHandshakeResponse41(void)
{
	SmartBuffer pkt;
	if (!receivePacket(pkt))
		return false;

	uint16_t *capabilityLsb = pkt.getPointer<uint16_t>();
	if (!((*capabilityLsb) & CLIENT_PROTOCOL_41)) {
		MLPL_CRIT("Currently client protocol other than 4.1 has "
		          "not been implented.\n");
		return false;
	}

	// fill the structure
	m_hsResp41.capability    = pkt.getValueAndIncIndex<uint32_t>();
	m_hsResp41.maxPacketSize = pkt.getValueAndIncIndex<uint32_t>();
	m_hsResp41.characterSet  = pkt.getValueAndIncIndex<uint8_t>();
	pkt.incIndex(HANDSHAKE_RESPONSE41_RESERVED_SIZE);
	m_hsResp41.username = getNullTermStringAndIncIndex(pkt);
	if (m_hsResp41.capability & CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) {
		uint64_t size = decodeLenEncInt(pkt);
		m_hsResp41.lenAuthResponse = size;
		m_hsResp41.authResponse =
		  getFixedLengthStringAndIncIndex(pkt, size);
	} else if (m_hsResp41.capability & CLIENT_SECURE_CONNECTION) {
		uint8_t size = pkt.getValueAndIncIndex<uint8_t>();
		m_hsResp41.lenAuthResponse = size;
		m_hsResp41.authResponse =
		  getFixedLengthStringAndIncIndex(pkt, size);
	} else
		m_hsResp41.authResponse = getNullTermStringAndIncIndex(pkt);

	if (m_hsResp41.capability & CLIENT_CONNECT_WITH_DB)
		m_hsResp41.database = getNullTermStringAndIncIndex(pkt);

	if (m_hsResp41.capability & CLIENT_PLUGIN_AUTH)
		m_hsResp41.authPluginName = getNullTermStringAndIncIndex(pkt);

	if (m_hsResp41.capability & CLIENT_CONNECT_ATTRS) {
		m_hsResp41.lenKeyValue = decodeLenEncInt(pkt);
		for (int i = 0; i < m_hsResp41.lenKeyValue; i++) {
			string key = decodeLenEncStr(pkt);
			string value = decodeLenEncStr(pkt);
			m_hsResp41.keyValueMap[key] = value;
		}
	}

	return true;
}

bool FaceMySQLWorker::receivePacket(SmartBuffer &pkt)
{
	uint32_t pktHeader;
	if (!receive(reinterpret_cast<char *>(&pktHeader), sizeof(pktHeader)))
		return false;
	m_packetId = (pktHeader & PACKET_ID_MASK) >> PACKET_ID_SHIFT_BITS;
	uint32_t pktSize = pktHeader & PACKET_SIZE_MASK;
	if (pktSize == 0xffffff) {
		MLPL_BUG("pktSize: 0xffffff: Not implemented\n");
		return false;
	}
	MLPL_INFO("pktSize: %08x (%d)\n", pktSize, m_packetId);

	// read response body
	pkt.alloc(pktSize);
	if (!receive(pkt, pktSize))
		return false;

	pkt.resetIndex();
	return true;
}

bool FaceMySQLWorker::receiveRequest(void)
{
	SmartBuffer pkt;
	if (!receivePacket(pkt))
		return false;
	uint8_t commandId = pkt.getValueAndIncIndex<uint8_t>();
	CommandProcFuncMapIterator it = m_cmdProcMap.find(commandId);
	if (it == m_cmdProcMap.end()) {
		MLPL_BUG("commandId: %02x: Not implemented\n", commandId);
		return false;
	}

	commandProcFunc &proc = it->second;
	return (this->*proc)(pkt);
}

bool FaceMySQLWorker::sendColumnDefinition41(
  string &schema, string &table, string &orgTable,
  string &name, string &orgName, uint32_t columnLength, uint8_t type,
  uint16_t flags, uint8_t decimals)
{
	const int initialPacketSize = 0x100;
	SmartBuffer pkt;
	allocAndAddPacketHeaderRegion(pkt, initialPacketSize);

	string catalog("def");
	addLenEncStr(pkt, catalog);
	addLenEncStr(pkt, schema);
	addLenEncStr(pkt, table);
	addLenEncStr(pkt, orgTable);
	addLenEncStr(pkt, name);
	addLenEncStr(pkt, orgName);

	const size_t lenFields = 0x0c;
	addLenEncInt(pkt, lenFields);
	pkt.ensureRemainingSize(lenFields);
	pkt.add16(m_charSet);
	pkt.add32(columnLength);
	pkt.add8(type);
	pkt.add16(flags);
	pkt.add8(decimals);
	pkt.addZero(2); // Filler

	//  if command was COM_FIELD_LIST 
	//  lenenc_int     length of default-values
	//  string[$len]   default values

	return sendPacket(pkt);
}

bool FaceMySQLWorker::sendSelectResult(SQLSelectResult &result)
{
	// Column Definition
	uint16_t flags = 0;
	uint8_t decimals = 0;
	for (size_t i = 0; i < result.columnDefs.size(); i++) {
		bool ret;
		SQLColumnDefinition &colDef = result.columnDefs[i];

		int type = typeConvert(colDef.type);
		if (type == TYPE_VAR_UNKNOWN) {
			MLPL_BUG("Failed to convert type: %d\n", colDef.type);
			return false;
		}
		ret = sendColumnDefinition41(colDef.schema,
		                             colDef.tableVar, colDef.table,
		                             colDef.columnVar, colDef.column,
		                             colDef.columnLength,
		                             type,
		                             flags, decimals);
		if (!ret)
			return false;
	}

	// EOF
	uint16_t status = m_statusFlags;
	if (!result.useIndex)
		status |= SERVER_STATUS_NO_INDEX_USED;
	if (!sendEOF(0, status))
		return false;

	for (size_t i = 0; i < result.rows.size(); i++) {
		// TODO: send content
	}

	// EOF
	if (!sendEOF(0, status))
		return false;

	return true;
}

bool FaceMySQLWorker::sendOK(uint64_t affectedRows, uint64_t lastInsertId)
{
	SmartBuffer pkt(11);
	addPacketHeaderRegion(pkt);
	pkt.add8(OK_HEADER);
	addLenEncInt(pkt, affectedRows);
	addLenEncInt(pkt, lastInsertId);

	if (m_hsResp41.capability & CLIENT_PROTOCOL_41) {
		pkt.add16(m_statusFlags);
		uint16_t warnings = 0;
		pkt.add16(warnings);
	} else if (m_hsResp41.capability & CLIENT_TRANSACTIONS) {
		MLPL_BUG("Not implemented\n");
	}
	// string[EOF]    info

	return sendPacket(pkt);
}

bool FaceMySQLWorker::sendEOF(uint16_t warningCount, uint16_t status)
{
	static const size_t EOF_PACKET_SIZE = 5;
	SmartBuffer pkt;
	allocAndAddPacketHeaderRegion(pkt, EOF_PACKET_SIZE);
	pkt.add8(EOF_HEADER);
	pkt.add16(warningCount);
	pkt.add16(status);
	return sendPacket(pkt);
}

bool FaceMySQLWorker::sendNull(void)
{
	SmartBuffer pkt;
	allocAndAddPacketHeaderRegion(pkt, 1);
	pkt.add8(RESULT_SET_ROW_NULL);
	return sendPacket(pkt);
}

bool FaceMySQLWorker::receive(char* buf, size_t size)
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

bool FaceMySQLWorker::sendLenEncInt(uint64_t num)
{
	SmartBuffer pkt;
	allocAndAddPacketHeaderRegion(pkt, MAX_LENENC_INT_LENGTH);
	addLenEncInt(pkt, num);
	return sendPacket(pkt);
}

bool FaceMySQLWorker::sendLenEncStr(string &str)
{
	SmartBuffer pkt;
	allocAndAddPacketHeaderRegion(pkt, str.size() + MAX_LENENC_INT_LENGTH);
	addLenEncStr(pkt, str);
	return sendPacket(pkt);
}

bool FaceMySQLWorker::sendPacket(SmartBuffer &pkt)
{
	m_packetId++;
	pkt.setAt(0, makePacketHeader(pkt.index() - PACKET_HEADER_SIZE));
	return send(pkt);
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

uint64_t FaceMySQLWorker::decodeLenEncInt(SmartBuffer &buf)
{
	uint8_t firstByte = buf.getValueAndIncIndex<uint8_t>();
	if (firstByte < 0xfc)
		return firstByte;
	if (firstByte == 0xfc) {
		return buf.getValueAndIncIndex<uint16_t>();
	}
	if (firstByte == 0xfd) {
		uint32_t v = buf.getValueAndIncIndex<uint8_t>();
		v <<= 16;
		v += buf.getValueAndIncIndex<uint16_t>();
		return v;
	}
	if (firstByte == 0xfe)
		return buf.getValueAndIncIndex<uint64_t>();

	MLPL_BUG("Not implemented: firstByte: %02x\n", firstByte); 
	return 0;
}

string FaceMySQLWorker::decodeLenEncStr(SmartBuffer &buf)
{
	uint64_t len = decodeLenEncInt(buf);
	return getFixedLengthStringAndIncIndex(buf, len);
}

string FaceMySQLWorker::getNullTermStringAndIncIndex(SmartBuffer &buf)
{
	string str(buf.getPointer<char>());
	buf.incIndex(str.size() + 1);
	return str;
}

string FaceMySQLWorker::getEOFString(SmartBuffer &buf)
{
	size_t size = buf.size() - buf.index();
	string str(buf.getPointer<char>(), size);
	return str;
}

string FaceMySQLWorker::getFixedLengthStringAndIncIndex(SmartBuffer &buf,
                                                        uint64_t length)
{
	string str(buf.getPointer<char>(), length);
	buf.incIndex(length);
	return str;
}

int FaceMySQLWorker::typeConvert(SQLColumnType type)
{
	if (type >= TYPE_CONVERT_TABLE_SIZE) {
		MLPL_BUG("type: %d > TYPE_CONVERT_TABLE_SIZE\n", type);
		return TYPE_VAR_UNKNOWN;
	}
	return m_typeConverTable[type];
}

// ---------------------------------------------------------------------------
// Command handlers
// ---------------------------------------------------------------------------
bool FaceMySQLWorker::comQuit(SmartBuffer &pkt)
{
	MLPL_DBG("%s\n", __PRETTY_FUNCTION__);
	return false;
}

bool FaceMySQLWorker::comQuery(SmartBuffer &pkt)
{
	ParsableString query(getEOFString(pkt));
	MLPL_DBG("******* %s: %s\n", __PRETTY_FUNCTION__, query.getString());

	string firstWord = query.readWord(ParsableString::SEPARATOR_SPACE);
	if (firstWord.empty()) {
		MLPL_WARN("Invalid query: '%s'\n", query.getString());
		return true;
	}

	transform(firstWord.begin(), firstWord.end(),
	          firstWord.begin(), ::tolower);
	QueryProcFuncMapIterator it = m_queryProcMap.find(firstWord);
	if (it == m_queryProcMap.end()) {
		MLPL_BUG("Not implemented: query command: '%s'\n",
		         query.getString());
		return false;
	}

	queryProcFunc &proc = it->second;
	return (this->*proc)(query);
}

bool FaceMySQLWorker::comInitDB(SmartBuffer &pkt)
{
	string DBname = getEOFString(pkt);
	MLPL_DBG("******* %s: %s\n", __PRETTY_FUNCTION__, DBname.c_str());
	m_currDBname = DBname;

	map<string, SQLProcessor *>::iterator it = m_sqlProcessorMap.find(DBname);
	if (it != m_sqlProcessorMap.end()) {
		m_sqlProcessor = it->second;
	} else {
		SQLProcessor *processor = SQLProcessorFactory::create(DBname);
		if (!processor) {
			MLPL_ERR("Unknown DB: %s\n", DBname.c_str());
			// TODO: should send Error?
		}
		m_sqlProcessor = processor;
		m_sqlProcessorMap[DBname] = m_sqlProcessor;
	}
	return sendOK();
}

bool FaceMySQLWorker::comSetOption(SmartBuffer &pkt)
{
	const size_t LEN_PKT_ARG = 2;
	if (pkt.remainingSize() < LEN_PKT_ARG) {
		MLPL_ERR("Invalid: packet size: %zd\n", pkt.remainingSize());
		return true;
	}
	m_mysql_option = pkt.getValueAndIncIndex<uint16_t>();
	MLPL_DBG("MYSQL_OPTION_MULTI_STATEMENTS: %d\n", m_mysql_option);
	return sendEOF(0, m_statusFlags);
}

// ---------------------------------------------------------------------------
// Query handlers
// ---------------------------------------------------------------------------
bool FaceMySQLWorker::querySelect(ParsableString &query)
{
	string column = query.readWord(m_separatorSpaceComma, true);

	// check if the specified column is the system variable 
	map<string, SelectProcFunc>::iterator it;
	it = m_selectProcFuncMap.find(column);
	if (it != m_selectProcFuncMap.end()) {
		SelectProcFunc &func = it->second;
		return (this->*func)(query);
	}

	if (m_sqlProcessor) {
		SQLSelectInfo selectInfo(query);
		SQLSelectResult result;
		if (m_sqlProcessor->select(result, selectInfo))
			return sendSelectResult(result);
	}
	MLPL_BUG("Not implemented: select: '%s'\n", query.getString());
	return false;
}

bool FaceMySQLWorker::querySet(ParsableString &query)
{
	MLPL_WARN("Not implemented: set: %s (ignored)\n", query.getString());
	return sendOK();
}

// ---------------------------------------------------------------------------
// System select handlers
// ---------------------------------------------------------------------------
bool FaceMySQLWorker::querySelectVersionComment(ParsableString &query)
{
	// [REF] http://dev.mysql.com/doc/internals/en/text-protocol.html#packet-COM_QUERY

	static const char VERSION_COMMENT[] = "ASURA";

	// Number of columns
	int numColum = 1;
	if (!sendLenEncInt(numColum))
		return false;

	// Column Definition
	string dummyStr;
	bool ret;
	string name = "@@version_comment";
	uint32_t columnLength = 8;
	uint8_t decimals = 0x1f;
	uint16_t flags = 0;
	ret = sendColumnDefinition41(dummyStr, dummyStr, dummyStr,
	                             name, dummyStr, columnLength,
	                             TYPE_VAR_STRING, flags,
	                             decimals);
	if (!ret)
		return false;

	// EOF
	if (!sendEOF(0, m_statusFlags))
		return false;

	// Row
	string versionComment(VERSION_COMMENT);
	if (!sendLenEncStr(versionComment))
		return false;

	// EOF
	if (!sendEOF(0, m_statusFlags))
		return false;

	return true;
}

bool FaceMySQLWorker::querySelectDatabase(ParsableString &query)
{
	// Number of columns
	int numColum = 1;
	if (!sendLenEncInt(numColum))
		return false;

	// Column Definition
	string dummyStr;
	bool ret;
	string name = "DATABASE()";
	uint32_t columnLength = 0x22;
	uint8_t decimals = 0x1f;
	uint16_t flags = 0;
	ret = sendColumnDefinition41(dummyStr, dummyStr, dummyStr,
	                             name, dummyStr, columnLength,
	                             TYPE_VAR_STRING, flags,
	                             decimals);
	if (!ret)
		return false;

	// EOF
	if (!sendEOF(0, m_statusFlags))
		return false;

	// Row
	if (!sendNull())
		return false;

	// EOF
	if (!sendEOF(0, m_statusFlags))
		return false;

	return true;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
void FaceMySQLWorker::initHandshakeResponse41(HandshakeResponse41 &hsResp41)
{
	hsResp41.capability = 0;
	hsResp41.maxPacketSize = 0;
	hsResp41.characterSet = 0;
	hsResp41.lenAuthResponse = 0;
	hsResp41.lenKeyValue = 0;
}
