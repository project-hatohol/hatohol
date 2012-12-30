#ifndef ArmZabbixAPI_h
#define ArmZabbixAPI_h

#include <libsoup/soup.h>
#include "ArmBase.h"

class ArmZabbixAPI : public ArmBase
{
public:
	ArmZabbixAPI(CommandLineArg &cmdArg);

protected:
	string getInitialJsonRequest(void);
	bool parseInitialResponse(SoupMessage *msg);
	bool mainThreadOneProc(void);

	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);

private:
	string m_server;
	string m_auth_token;
	string m_uri;
	int    m_server_port;
	int    m_retry_interval;	// in sec
	int    m_repeat_interval;	// in sec;
};

#endif // ArmZabbixAPI_h

