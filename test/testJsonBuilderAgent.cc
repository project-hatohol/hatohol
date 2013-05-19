#include <cppcutter.h>
#include "JsonBuilderAgent.h"

namespace testJsonBuilderAgent {

// -------------------------------------------------------------------------
// test cases
// -------------------------------------------------------------------------
void test_parseString(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.endObject();
	string expected = "{}";
	cppcut_assert_equal(expected, agent.generate());
}

} //namespace testJsonBuilderAgent


