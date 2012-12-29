#ifndef FaceMySQLWorker_h
#define FaceMySQLWorker_h

#include <map>
using namespace std;

#include <glib.h>
#include <gio/gio.h>

#include <SmartBuffer.h>
using namespace mlpl;

#include "Utils.h"

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

class FaceMySQLWorker {
public:
	FaceMySQLWorker(GSocket *sock, uint32_t connId);
	virtual ~FaceMySQLWorker();
	void start(void);

protected:
	gpointer mainThread(void);
	uint32_t makePacketHeader(uint32_t length);
	void addPacketHeaderRegion(SmartBuffer &pkt);
	void allocAndAddPacketHeaderRegion(SmartBuffer &pkt, size_t packetSize);
	void addLenEncInt(SmartBuffer &buf, uint64_t num);
	void addLenEncStr(SmartBuffer &pkt, string &str);
	bool sendHandshakeV10(void);
	bool receiveHandshakeResponse41(void);
	bool receivePacket(SmartBuffer &pkt);
	bool receiveRequest(void);
	bool sendOK(void);
	bool sendColumnDefinition41(
	  string &schema, string &table, string &orgTable,
	  string &name, string &orgName, uint32_t columnLength, uint8_t type,
	  uint16_t flags, uint8_t decimals);
	bool sendLenEncInt(uint64_t num);
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
	bool comQuerySelect(string &query, vector<string> &words);
	bool comQuerySelectVersionComment(string &query, vector<string> &words);

private:
	GThread *m_thread;
	GSocket *m_socket;
	uint32_t m_connId;
	uint32_t m_packetId;
	uint32_t m_charSet;
	HandshakeResponse41 m_hsResp41;

	static gpointer _mainThread(gpointer data);
	void initHandshakeResponse41(HandshakeResponse41 &hsResp41);
};

#endif // FaceMySQLWorker_h
