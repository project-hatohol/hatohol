#ifndef FaceMySQLWorker_h
#define FaceMySQLWorker_h

#include <map>
using namespace std;

#include <glib.h>
#include <gio/gio.h>

#include <SmartBuffer.h>
using namespace mlpl;

#include "Utils.h"
#include "AsuraThreadBase.h"
#include "SQLProcessor.h"

struct HandshakeResponse41
{
	uint32_t capability;
	uint32_t maxPacketSize;
	uint8_t  characterSet;
	string   username;
	uint16_t lenAuthResponse;
	string   authResponse;

	//  if capabilities & CLIENT_CONNECT_WITH_DB
	string   database;

	// if capabilities & CLIENT_PLUGIN_AUTH
	string authPluginName;

	// if capabilities & CLIENT_CONNECT_ATTRS
	uint16_t lenKeyValue;
	map<string, string> keyValueMap;
};

class FaceMySQLWorker;
typedef bool (FaceMySQLWorker::*commandProcFunc)(SmartBuffer &pkt);
typedef map<int, commandProcFunc> CommandProcFuncMap;
typedef CommandProcFuncMap::iterator CommandProcFuncMapIterator;

typedef bool (FaceMySQLWorker::*queryProcFunc)(string &query, vector<string> &words);
typedef map<string, queryProcFunc> QueryProcFuncMap;
typedef QueryProcFuncMap::iterator QueryProcFuncMapIterator;

class FaceMySQLWorker : public AsuraThreadBase {
public:
	static void init(void);
	FaceMySQLWorker(GSocket *sock, uint32_t connId);
	virtual ~FaceMySQLWorker();

protected:
	uint32_t makePacketHeader(uint32_t length);
	void addPacketHeaderRegion(SmartBuffer &pkt);
	void allocAndAddPacketHeaderRegion(SmartBuffer &pkt, size_t packetSize);
	void addLenEncInt(SmartBuffer &buf, uint64_t num);
	void addLenEncStr(SmartBuffer &pkt, string &str);
	bool sendHandshakeV10(void);
	bool receiveHandshakeResponse41(void);
	bool receivePacket(SmartBuffer &pkt);

	/**
	 * When this function returns false, mainThread() exits from the loop.
	 * So 'true' will be returned except for fatal cases.
	 */
	bool receiveRequest(void);
	bool sendOK(uint64_t affectedRows = 0, uint64_t lastInsertId = 0);
	bool sendEOF(uint16_t warningCount, uint16_t status);
	bool sendColumnDefinition41(
	  string &schema, string &table, string &orgTable,
	  string &name, string &orgName, uint32_t columnLength, uint8_t type,
	  uint16_t flags, uint8_t decimals);
	bool sendSelectResult(SQLSelectResult &result);
	bool sendLenEncInt(uint64_t num);
	bool sendLenEncStr(string &str);
	bool sendPacket(SmartBuffer &pkt);
	bool send(SmartBuffer &buf);
	bool receive(char* buf, size_t size);
	uint64_t decodeLenEncInt(SmartBuffer &buf);
	string   decodeLenEncStr(SmartBuffer &buf);
	string getNullTermStringAndIncIndex(SmartBuffer &buf);
	string getEOFString(SmartBuffer &buf);
	string getFixedLengthStringAndIncIndex(SmartBuffer &buf,
	                                       uint64_t length);
	bool comQuery(SmartBuffer &pkt);
	bool comInitDB(SmartBuffer &pkt);
	bool comSetOption(SmartBuffer &pkt);

	bool querySelect(string &query, vector<string> &words);
	bool querySet(string &query, vector<string> &words);

	bool querySelectVersionComment(string &query, vector<string> &words);
	int typeConvert(SQLColumnType type);

	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);
private:
	static const size_t TYPE_CONVERT_TABLE_SIZE = 0x100;
	static int m_typeConverTable[TYPE_CONVERT_TABLE_SIZE];

	GSocket *m_socket;
	uint32_t m_connId;
	uint32_t m_packetId;
	uint32_t m_charSet;
	string   m_currDBname;
	uint16_t m_mysql_option;
	uint16_t m_statusFlags;
	HandshakeResponse41 m_hsResp41;
	CommandProcFuncMap m_cmdProcMap;
	QueryProcFuncMap   m_queryProcMap;
	SQLProcessor        *m_sqlProcessor;
	map<string, SQLProcessor *> m_sqlProcessorMap;

	void initHandshakeResponse41(HandshakeResponse41 &hsResp41);
};

#endif // FaceMySQLWorker_h
