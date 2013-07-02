#include <cppcutter.h>
#include <sys/time.h>
#include <errno.h>
#include "Hatohol.h"
#include "ZabbixAPIEmulator.h"
#include "ArmZabbixAPI.h"
#include "ArmZabbixAPI-template.h"
#include "Helpers.h"
#include "Synchronizer.h"
#include "DBClientZabbix.h"
#include "ConfigManager.h"
#include "ItemTablePtr.h"
#include "JsonParserAgent.h"

namespace testArmZabbixAPI {
static Synchronizer g_sync;

static const size_t NUM_TEST_READ_TIMES = 10;

static MonitoringServerInfo g_defaultServerInfo =
{
	0,                        // id
	MONITORING_SYSTEM_ZABBIX, // type
	"127.0.0.1",              // hostname
	"127.0.0.1",              // ip_address
	"No name",                // nickname
	0,                        // port
	10,                       // polling_interval_sec
	5,                        // retry_interval_sec
	"foo",                      // username
	"bar",                      // password
};


class ArmZabbixAPITestee :  public ArmZabbixAPI {

typedef bool (ArmZabbixAPITestee::*ThreadOneProc)(void);

public:
	enum GetTestType {
		GET_TEST_TYPE_TRIGGERS,
		GET_TEST_TYPE_FUNCTIONS,
		GET_TEST_TYPE_ITEMS,
		GET_TEST_TYPE_HOSTS,
		GET_TEST_TYPE_APPLICATIONS,
		GET_TEST_TYPE_EVENTS,
	};

	ArmZabbixAPITestee(const MonitoringServerInfo &serverInfo)
	: ArmZabbixAPI(serverInfo),
	  m_result(false),
	  m_threadOneProc(&ArmZabbixAPITestee::defaultThreadOneProc),
	  m_countThreadOneProc(0),
	  m_repeatThreadOneProc(1)
	{
		addExceptionCallback(_exceptionCb, this);
	}

	bool getResult(void) const
	{
		return m_result;
	}

	const string errorMessage(void) const
	{
		return m_errorMessage;
	}

	bool testOpenSession(void)
	{
		g_sync.lock();
		setPollingInterval(0);
		m_threadOneProc = &ArmZabbixAPITestee::threadOneProcOpenSession;
		addExitCallback(exitCbDefault, this);
		start();
		g_sync.wait();
		return m_result;
	}

	bool testGet(GetTestType type)
	{
		bool succeeded = false;
		if (type == GET_TEST_TYPE_TRIGGERS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcTriggers,
			         exitCbDefault, this);
		} else if (type == GET_TEST_TYPE_FUNCTIONS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcFunctions,
			         exitCbDefault, this);
		} else if (type == GET_TEST_TYPE_ITEMS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcItems,
			         exitCbDefault, this);
		} else if (type == GET_TEST_TYPE_HOSTS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcHosts,
			         exitCbDefault, this, 1);
		} else if (type == GET_TEST_TYPE_APPLICATIONS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcApplications,
			         exitCbDefault, this, 1);
		} else if (type == GET_TEST_TYPE_EVENTS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcEvents,
			         exitCbDefault, this);
		} else {
			cut_fail("Unknown get test type: %d\n", type);
		}
		return succeeded;
	}

	bool testMainThreadOneProc(void)
	{
		// This test is typically executed without the start of
		// the thread of ArmZabbixAPI. The exit callback (that unlock
		// mutex in ArmZabbixAPI) is never called. So we explicitly
		// call exitCallbackFunc() here to unlock the mutex;
		return ArmZabbixAPI::mainThreadOneProc();
	}

	string startInitialJsonRequest(void)
	{
		return ArmZabbixAPI::getInitialJsonRequest();
	}

	void assertMakeItemVector(bool testNull = false)
	{
		// make test data and call the target method.
		const ItemId pickupItemId = 1;
		const size_t numTestData = 10;
		vector<int> itemVector;
		vector<int> expectedItemVector;
		VariableItemTablePtr table;
		for (size_t i = 0; i < numTestData; i++) {
			VariableItemGroupPtr grp;
			ItemInt *item = new ItemInt(pickupItemId, i);
			if (testNull && i % 2 == 0)
				item->setNull();
			else
				expectedItemVector.push_back(i);
			grp->add(item, false);
			table->add(grp);
		}
		makeItemVector<int>(itemVector, table, pickupItemId);
		
		// check
		cppcut_assert_equal(expectedItemVector.size(),
		                    itemVector.size());
		for (size_t i = 0; i < expectedItemVector.size(); i++) {
			int expected = expectedItemVector[i];
			int actual = itemVector[i];
			cppcut_assert_equal(expected, actual);
		}
	}

protected:
	static void _exceptionCb(const exception &e, void *data)
	{
		ArmZabbixAPITestee *obj =
		   static_cast<ArmZabbixAPITestee *>(data);
		obj->exceptionCb(e);
	}

	static void exitCbDefault(void *)
	{
		g_sync.unlock();
	}

	bool launch(ThreadOneProc threadOneProc, 
	            void (*exitCb)(void *), void *exitCbData,
	            size_t numRepeat = NUM_TEST_READ_TIMES)
	{
		m_countThreadOneProc = 0;
		m_repeatThreadOneProc = numRepeat;
		g_sync.lock();
		setPollingInterval(0);
		m_threadOneProc = threadOneProc;
		addExitCallback(exitCb, exitCbData);
		start();
		g_sync.wait();
		return m_result;
	}

	void exceptionCb(const exception &e)
	{
		m_result = false;
		m_errorMessage = e.what();
		g_sync.unlock();
	}

	bool defaultThreadOneProc(void)
	{
		THROW_HATOHOL_EXCEPTION("m_threadOneProc is not set.");
		return false;
	}

	bool threadOneProcOpenSession(void)
	{
		bool succeeded = openSession();
		if (!succeeded)
			m_errorMessage = "Failed: mainThreadOneProc()";
		m_result = succeeded;
		requestExit();
		return succeeded;
	}

	bool threadOneProcTriggers(void)
	{
		updateTriggers();
		return true;
	}

	bool threadOneProcFunctions(void)
	{
		// updateTriggers() is neccessary before updateFunctions(),
		// because 'function' information is obtained at the same time
		// as a response of 'triggers.get' request.
		updateTriggers();
		updateFunctions();
		return true;
	}

	bool threadOneProcItems(void)
	{
		updateItems();
		return true;
	}

	bool threadOneProcHosts(void)
	{
		updateHosts();
		return true;
	}

	bool threadOneProcApplications(void)
	{
		updateApplications();
		return true;
	}

	bool threadOneProcEvents(void)
	{
		updateEvents();
		return true;
	}

	// virtual function
	bool mainThreadOneProc(void)
	{
		if (!openSession()) {
			requestExit();
			return false;
		}
		if (!(this->*m_threadOneProc)()) {
			requestExit();
			return false;
		}
		m_countThreadOneProc++;
		if (m_countThreadOneProc++ >= m_repeatThreadOneProc) {
			m_result = true;
			requestExit();
		}
		return true;
	}

private:
	bool m_result;
	string m_errorMessage;
	ThreadOneProc m_threadOneProc;
	size_t        m_countThreadOneProc;
	size_t        m_repeatThreadOneProc;
};

static const guint EMULATOR_PORT = 33333;

ZabbixAPIEmulator g_apiEmulator;

static guint getTestPort(void)
{
	static guint port = 0;
	if (port != 0)
		return port;

	char *env = getenv("TEST_ARM_ZABBIX_API_PORT");
	if (!env) {
		port = EMULATOR_PORT;
		return port;
	}

	guint _port = atoi(env);
	if (_port > 65535)
		cut_fail("Invalid string: %s", env);
	port = _port;
	return port;
}

static void _assertReceiveData(ArmZabbixAPITestee::GetTestType testType,
                               int svId)
{
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();

	deleteDBClientZabbixDB(svId);
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal
	  (true, armZbxApiTestee.testGet(testType),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}
#define assertReceiveData(TYPE, SERVER_ID) \
cut_trace(_assertReceiveData(TYPE, SERVER_ID))

static void _assertTestGet(ArmZabbixAPITestee::GetTestType testType)
{
	int svId = 0;
	assertReceiveData(testType, svId);

	// TODO: add the check of the database
}
#define assertTestGet(TYPE) cut_trace(_assertTestGet(TYPE))

void setup(void)
{
	cppcut_assert_equal(false, g_sync.isLocked(),
	                    cut_message("g_sync is locked."));

	hatoholInit();
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start(EMULATOR_PORT);
	else
		g_apiEmulator.setOperationMode(OPE_MODE_NORMAL);
}

void teardown(void)
{
	g_sync.reset();
	g_apiEmulator.reset();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_openSession(void)
{
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();

	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal
	  (true, armZbxApiTestee.testOpenSession(),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}

void test_getTriggers(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_TRIGGERS);
}

void test_getFunctions(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_FUNCTIONS);
}

void test_getItems(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_ITEMS);
}

void test_getHosts(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_HOSTS);
}

void test_getEvents(void)
{
	// We expect empty data for the last two times.
	g_apiEmulator.setNumberOfEventSlices(NUM_TEST_READ_TIMES-2);
	assertReceiveData(ArmZabbixAPITestee::GET_TEST_TYPE_EVENTS, 0);
}

void test_getApplications(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_APPLICATIONS);
}

void test_httpNotFound(void)
{
	if (getTestPort() != EMULATOR_PORT)
		cut_pend("Test port is not emulator port.");

	g_apiEmulator.setOperationMode(OPE_MODE_HTTP_NOT_FOUND);
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal
	  (false, armZbxApiTestee.testOpenSession(),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}

void test_mainThreadOneProc()
{
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	deleteDBClientZabbixDB(svId);
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal(true, armZbxApiTestee.testMainThreadOneProc());
}

void test_makeItemVecotr(void)
{
	ArmZabbixAPITestee armZbxApiTestee(g_defaultServerInfo);
	armZbxApiTestee.assertMakeItemVector();
}

void test_makeItemVecotrWithNullValue(void)
{
	ArmZabbixAPITestee armZbxApiTestee(g_defaultServerInfo);
	armZbxApiTestee.assertMakeItemVector(true);
}

void test_checkUsernamePassword(void)
{
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	JsonParserAgent parser(armZbxApiTestee.startInitialJsonRequest());
	string jsonUserName;
	string jsonPassword;
	cppcut_assert_equal(false, parser.hasError());
	cppcut_assert_equal(true, parser.startObject("params"));
	cppcut_assert_equal(true, parser.read("user", jsonUserName));
	cppcut_assert_equal(true, parser.read("password", jsonPassword));
	cppcut_assert_equal(serverInfo.userName, jsonUserName);
	cppcut_assert_equal(serverInfo.password, jsonPassword);
}
} // namespace testArmZabbixAPI
