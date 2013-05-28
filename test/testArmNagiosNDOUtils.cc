#include <cppcutter.h>
#include "Asura.h"
#include "ArmNagiosNDOUtils.h"

namespace testArmNagiosNDOUtils {

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_create(void)
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
		ArmNagiosNDOUtils armNagios(serverInfo);
	} catch (const exception &e) {
		gotException = true;
		cut_fail("Got exception: %s", e.what());
	}
	cppcut_assert_equal(false, gotException);
}

} // namespace testArmNagiosNDOUtils

