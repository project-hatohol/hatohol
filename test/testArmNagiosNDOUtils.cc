#include <cppcutter.h>
#include "Asura.h"
#include "ArmNagiosNDOUtils.h"

namespace testArmNagiosNDOUtils {

static ArmNagiosNDOUtils *g_armNagi = NULL;

static void createGlobalInstance(void)
{
	MonitoringServerInfo serverInfo;
	serverInfo.id = 1;
	serverInfo.type = MONITORING_SYSTEM_NAGIOS;
	serverInfo.hostName = "localhost";
	serverInfo.ipAddress = "127.0.0.1";
	serverInfo.nickname = "My PC";
	serverInfo.port = 0; // default port
	serverInfo.pollingIntervalSec = 10;
	serverInfo.retryIntervalSec = 30;
	serverInfo.userName = "ndoutils";
	serverInfo.password = "admin";
	serverInfo.dbName   = "ndoutils";
	bool gotException = false;
	try {
		g_armNagi = new ArmNagiosNDOUtils(serverInfo);
	} catch (const exception &e) {
		gotException = true;
		cut_fail("Got exception: %s", e.what());
	}
	cppcut_assert_equal(false, gotException);
}

void setup(void)
{
	asuraInit();
	createGlobalInstance();
}

void teardown(void)
{
	if (g_armNagi) {
		delete g_armNagi;
		g_armNagi = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_create(void)
{
	cppcut_assert_not_null(g_armNagi);
}

} // namespace testArmNagiosNDOUtils

