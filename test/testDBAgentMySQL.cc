#include <cppcutter.h>
#include "DBAgentMySQL.h"

namespace testDBAgentMySQL {

static const char *TEST_DB_NAME = "test_db_agent_mysql";

DBAgentMySQL *g_dbAgent = NULL;

static void _createGlobalDBAgent(void)
{
	try {
		g_dbAgent = new DBAgentMySQL(TEST_DB_NAME);
	} catch (const exception &e) {
		cut_fail("%s", e.what());
	}
}
#define createGlobalDBAgent() cut_trace(_createGlobalDBAgent())

void teardown(void)
{
	if (g_dbAgent) {
		delete g_dbAgent;
		g_dbAgent = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_create(void)
{
	createGlobalDBAgent();
}

} // testDBAgentMySQL

