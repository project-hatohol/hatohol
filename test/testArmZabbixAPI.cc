#include <cppcutter.h>
#include <sys/time.h>
#include <errno.h>
#include "Asura.h"
#include "ZabbixAPIEmulator.h"

namespace testArmZabbixAPI {

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
void test_getEvent(void)
{
	// dummy
}

} // namespace testArmZabbixAPI
