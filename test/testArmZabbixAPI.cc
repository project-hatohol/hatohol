#include <cppcutter.h>
#include <sys/time.h>
#include <errno.h>
#include "Asura.h"
#include "ZabbixAPIEmulator.h"
#include "ArmZabbixAPI.h"
#include "Helpers.h"

namespace testArmZabbixAPI {
static GMutex g_mutex;

static void wait_mutex(void)
{
	g_mutex_lock(&g_mutex);
	g_mutex_unlock(&g_mutex);
}

class ArmZabbixAPITestee :  public ArmZabbixAPI {
public:
	ArmZabbixAPITestee(int zabbixServerId, const char *server)
	: ArmZabbixAPI(zabbixServerId, server)
	{
		if (!g_mutex_trylock(&g_mutex))
			cut_fail("g_mutex is used.");
	}
};

static const guint EMULATOR_PORT = 33333;

ZabbixAPIEmulator g_apiEmulator;

void setup(void)
{
	asuraInit();
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start(EMULATOR_PORT);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getTriggers(void)
{
	int svId = 0;
	deleteDBClientZabbixDB(svId);
	ArmZabbixAPITestee armZbxApiTestee(svId, "localhost");
	armZbxApiTestee.start();
	wait_mutex();
}

} // namespace testArmZabbixAPI
