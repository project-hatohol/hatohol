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
	: ArmZabbixAPI(zabbixServerId, server),
	  m_result(false)
	{
		if (!g_mutex_trylock(&g_mutex))
			cut_fail("g_mutex is used.");
	}

	bool getResult(void) const
	{
		return m_result;
	}

	const string errorMessage(void) const
	{
		return m_errorMessage;
	}

	bool testGetTriggers(void)
	{
		setPollingInterval(0);
		start();
		wait_mutex();
		return false;
	}

protected:

private:
	bool m_result;
	string m_errorMessage;
};

static const guint EMULATOR_PORT = 33333;

ZabbixAPIEmulator g_apiEmulator;

void setup(void)
{
	asuraInit();
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start(EMULATOR_PORT);
}

void teardown(void)
{
	if (!g_mutex_trylock(&g_mutex))
		g_mutex_unlock(&g_mutex);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getTriggers(void)
{
	int svId = 0;
	deleteDBClientZabbixDB(svId);
	ArmZabbixAPITestee armZbxApiTestee(svId, "localhost");
	cppcut_assert_equal
	  (true, armZbxApiTestee.testGetTriggers(),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}

} // namespace testArmZabbixAPI
