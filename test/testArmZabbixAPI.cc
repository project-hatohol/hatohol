#include <cppcutter.h>
#include <sys/time.h>
#include <errno.h>
#include "Asura.h"
#include "ZabbixAPIEmulator.h"

namespace testArmZabbixAPI {

ZabbixAPIEmulator g_apiEmulator;

void setup(void)
{
	asuraInit();
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

} // namespace testArmZabbixAPI
