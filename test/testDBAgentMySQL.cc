#include <cppcutter.h>
#include "DBAgentMySQL.h"

namespace testDBAgentMySQL {

static const char *TEST_DB_NAME = "test_db_agent_mysql";

DBAgentMySQL *g_dbAgent = NULL;

static void _createDBAgent(void)
{
	try {
		g_dbAgent = new DBAgentMySQL(TEST_DB_NAME);
	} catch (const exception &e) {
		cut_fail("%s", e.what());
	}
}
#define createDBAgent() cut_trace(_createDBAgent())

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
	createDBAgent();
}

} // testDBAgentMySQL

