#include <cppcutter.h>
#include <sys/time.h>
#include <errno.h>
#include "Asura.h"
#include "ZabbixAPIEmulator.h"
#include "ArmZabbixAPI.h"
#include "Helpers.h"
#include "Synchronizer.h"
#include "DBClientZabbix.h"
#include "ConfigManager.h"

namespace testArmZabbixAPI {
static Synchronizer g_sync;

class ArmZabbixAPITestee :  public ArmZabbixAPI {

typedef bool (ArmZabbixAPITestee::*ThreadOneProc)(void);

static const size_t NUM_TEST_TRIGGER_READ = 10;

public:
	enum GetTestType {
		GET_TEST_TYPE_TRIGGERS,
		GET_TEST_TYPE_FUNCTIONS,
	};

	ArmZabbixAPITestee(int zabbixServerId, const char *server, int port)
	: ArmZabbixAPI(zabbixServerId, server, port),
	  m_result(false),
	  m_threadOneProc(&ArmZabbixAPITestee::defaultThreadOneProc),
	  m_countThreadOneProc(0)
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
		} else {
			cut_fail("Unknown get test type: %d\n", type);
		}
		return succeeded;
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
	            void (*exitCb)(void *), void *exitCbData)
	{
		m_countThreadOneProc = 0;
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
		THROW_ASURA_EXCEPTION("m_threadOneProc is not set.");
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
		if (m_countThreadOneProc++ >= NUM_TEST_TRIGGER_READ) {
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

static void _assertTestGet(ArmZabbixAPITestee::GetTestType testType,
                           int targetId)
{
	int svId = 0;
	deleteDBClientZabbixDB(svId);
	ArmZabbixAPITestee armZbxApiTestee(svId, "localhost", getTestPort());
	cppcut_assert_equal
	  (true, armZbxApiTestee.testGet(testType),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
	
	// check the database
	string statement = StringUtils::sprintf(
	  "select count(*) from replica_generation where target_id=%d",
	  targetId);
	
	string numGenerations = execSqlite3ForDBClinetZabbix(svId, statement);

	ConfigManager *confMgr = ConfigManager::getInstance();
	static size_t expectedNumGenerations =
	   confMgr->getNumberOfPreservedReplicaGeneration();
	cppcut_assert_equal(
	  StringUtils::sprintf("%zd\n", expectedNumGenerations),
	  numGenerations);
}
#define assertTestGet(TYPE, TARGET_ID) \
cut_trace(_assertTestGet(TYPE, TARGET_ID))

void setup(void)
{
	cppcut_assert_equal(false, g_sync.isLocked(),
	                    cut_message("g_sync is locked."));

	asuraInit();
	DBClientZabbix::resetDBInitializedFlags();
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start(EMULATOR_PORT);
	else
		g_apiEmulator.setOperationMode(OPE_MODE_NORMAL);
}

void teardown(void)
{
	g_sync.reset();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_openSession(void)
{
	int svId = 0;
	ArmZabbixAPITestee armZbxApiTestee(svId, "localhost", getTestPort());
	cppcut_assert_equal
	  (true, armZbxApiTestee.testOpenSession(),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}

void test_getTriggers(void)
{
	assertTestGet(
	  ArmZabbixAPITestee::GET_TEST_TYPE_TRIGGERS,
	  DBClientZabbix::REPLICA_GENERATION_TARGET_ID_TRIGGER);
}

void test_getFunctions(void)
{
	assertTestGet(
	  ArmZabbixAPITestee::GET_TEST_TYPE_FUNCTIONS,
	  DBClientZabbix::REPLICA_GENERATION_TARGET_ID_FUNCTION);
}

void test_httpNotFound(void)
{
	if (getTestPort() != EMULATOR_PORT)
		cut_pend("Test port is not emulator port.");

	g_apiEmulator.setOperationMode(OPE_MODE_HTTP_NOT_FOUND);
	int svId = 0;
	ArmZabbixAPITestee armZbxApiTestee(svId, "localhost", getTestPort());
	cppcut_assert_equal
	  (false, armZbxApiTestee.testOpenSession(),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}

} // namespace testArmZabbixAPI
