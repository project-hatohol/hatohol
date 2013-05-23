#include <cppcutter.h>
#include <StringUtils.h>
#include "JsonBuilderAgent.h"

using namespace mlpl;

namespace testJsonBuilderAgent {

// -------------------------------------------------------------------------
// test cases
// -------------------------------------------------------------------------
void test_emptyObject(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.endObject();
	string expected = "{}";
	cppcut_assert_equal(expected, agent.generate());
}

void test_array(void)
{
	string arrayName = "foo";
	const int numArray = 5;
	JsonBuilderAgent agent;
	agent.startObject();
	agent.startArray(arrayName);
	for (int i = 0; i < numArray; i++)
		agent.add(i);
	agent.endArray();
	agent.endObject();

	// check
	string expected = StringUtils::sprintf("{\"%s\":[", arrayName.c_str());
	for (int i = 0; i < numArray; i++) {
		if (i < numArray - 1)
			expected += StringUtils::sprintf("%d,", i);
		else
			expected += StringUtils::sprintf("%d]", i);
	}
	expected += "}";
	cppcut_assert_equal(expected, agent.generate());
}

} //namespace testJsonBuilderAgent


