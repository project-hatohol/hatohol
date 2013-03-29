#include <cppcutter.h>
#include "Helpers.h"

void _assertStringVector(StringVector &expected, StringVector &actual)
{
	cppcut_assert_equal(expected.size(), actual.size());
	for (size_t i = 0; i < expected.size(); i++)
		cppcut_assert_equal(expected[i], actual[i]);
}

string executeCommand(const string &commandLine)
{
	gboolean ret;
	gchar *standardOutput = NULL;
	gchar *standardError = NULL;
	gint exitStatus;
	GError *error = NULL;
	string errorStr;
	string stdoutStr, stderrStr;
	int version;

	ret = g_spawn_command_line_sync(commandLine.c_str(),
	                                &standardOutput, &standardError,
	                                &exitStatus, &error);
	if (standardOutput) {
		stdoutStr = standardOutput;
		g_free(standardOutput);
	}
	if (standardError) {
		stderrStr = standardError;
		g_free(standardError);
	}
	if (!ret)
		goto err;
	if (exitStatus != 0)
		goto err;
	return stdoutStr;

err:
	if (error) {
		errorStr = error->message;
		g_error_free(error);
	}
	cut_fail("ret: %d, exit status: %d\n<<stdout>>\n%s\n<<stderr>>\n%s\n"
	         "<<error->message>>\n%s",
	         ret, exitStatus, stdoutStr.c_str(), stderrStr.c_str(), 
	         errorStr.c_str());
}

