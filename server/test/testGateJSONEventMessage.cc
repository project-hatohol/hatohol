/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <gcutter.h>
#include <cppcutter.h>

#include <GateJSONEventMessage.h>

using namespace std;
using namespace mlpl;

namespace testGateJSONEventMessage {

GateJSONEventMessage *parseRaw(JsonParser *parser, const string &json)
{
	GError *error = NULL;
	json_parser_load_from_data(parser,
				   json.c_str(),
				   json.length(),
				   &error);
	gcut_assert_error(error);

	JsonNode *root;
	root = json_parser_get_root(parser);
	return new GateJSONEventMessage(root);
}

const GList *convertStringListToGList(StringList &strings)
{
	StringListIterator it = strings.begin();
	GList *stringList = NULL;
	for (; it != strings.end(); ++it) {
		string &element = *it;
		char *copiedElement = g_strdup(element.c_str());
		stringList = g_list_append(stringList, copiedElement);
	}
	return gcut_take_list(stringList, g_free);
}

namespace validate {
	JsonParser *parser;
	GateJSONEventMessage *message;
	StringList *errors;

	void cut_setup(void)
	{
		parser = json_parser_new();
		message = NULL;
		errors = new StringList();
	}

	void cut_teardown(void)
	{
		delete errors;
		delete message;
		g_object_unref(parser);
	}

	void parse(const string &json)
	{
		message = parseRaw(parser, json);
	}

	const GList *getErrorList()
	{
		return convertStringListToGList(*errors);
	}

	void _assertValidate(const GList *expectedErrorList,
			     const string &json)
	{
		parse(json);

		bool succeeded = message->validate(*errors);
		gcut_assert_equal_list_string(expectedErrorList,
					      getErrorList());
		if (errors->empty()) {
			cppcut_assert_equal(true, succeeded);
		} else {
			cppcut_assert_equal(false, succeeded);
		}
	}

#define assertValidate(expectedErrorList, json)			\
	cut_trace(_assertValidate(expectedErrorList, json))

	void test_valid(void)
	{
		assertValidate(NULL,
			       "{\n"
			       "  \"type\": \"event\",\n"
			       "  \"body\": {\n"
			       "    \"id\":        1,\n"
			       "    \"timestamp\": 1407824772.9396641,\n"
			       "    \"hostName\":  \"www.example.com\",\n"
			       "    \"content\":   \"Error!\"\n"
			       "  }\n"
			       "}\n");
	}

	void test_noType(void)
	{
		assertValidate(gcut_take_new_list_string(
				       "$.type must exist",
				       NULL),
			       "{\n"
			       "  \"body\": {\n"
			       "    \"id\":        1,\n"
			       "    \"timestamp\": 1407824772.9396641,\n"
			       "    \"hostName\":  \"www.example.com\",\n"
			       "    \"content\":   \"Error!\"\n"
			       "  }\n"
			       "}\n");
	}

	void test_unsupportedType(void)
	{
		assertValidate(gcut_take_new_list_string(
				       "$.type must be \"event\" for now",
				       NULL),
			       "{\n"
			       "  \"type\": \"unsupportedType\",\n"
			       "  \"body\": {\n"
			       "    \"id\":        1,\n"
			       "    \"timestamp\": 1407824772.9396641,\n"
			       "    \"hostName\":  \"www.example.com\",\n"
			       "    \"content\":   \"Error!\"\n"
			       "  }\n"
			       "}\n");
	}

	void test_noBody(void)
	{
		assertValidate(gcut_take_new_list_string(
				       "$.body must exist",
				       NULL),
			       "{\n"
			       "  \"type\": \"event\"\n"
			       "}\n");
	}

	void test_invalidBodyType(void)
	{
		assertValidate(gcut_take_new_list_string(
				       "$.body must be JsonObject: "
				       "gchararray <\"Error!\">",
				       NULL),
			       "{\n"
			       "  \"type\": \"event\"\n,"
			       "  \"body\": \"Error!\"\n"
			       "}\n");
	}

	void test_noID(void)
	{
		assertValidate(gcut_take_new_list_string(
				       "$.body.id must exist",
				       NULL),
			       "{\n"
			       "  \"type\": \"event\"\n,"
			       "  \"body\": {\n"
			       "    \"timestamp\": 1407824772.9396641,\n"
			       "    \"hostName\":  \"www.example.com\",\n"
			       "    \"content\":   \"Error!\"\n"
			       "  }\n"
			       "}\n");
	}

	void test_noTimestamp(void)
	{
		assertValidate(gcut_take_new_list_string(
				       "$.body.timestamp must exist",
				       NULL),
			       "{\n"
			       "  \"type\": \"event\"\n,"
			       "  \"body\": {\n"
			       "    \"id\":        1,\n"
			       "    \"hostName\":  \"www.example.com\",\n"
			       "    \"content\":   \"Error!\"\n"
			       "  }\n"
			       "}\n");
	}

	void test_invalidTimestamp(void)
	{
		assertValidate(gcut_take_new_list_string(
				       "$.body.timestamp must be "
				       "UNIX time in double or "
				       "ISO 8601 string: "
				       "<\"invalid timestamp\">",
				       NULL),
			       "{\n"
			       "  \"type\": \"event\"\n,"
			       "  \"body\": {\n"
			       "    \"id\":        1,\n"
			       "    \"timestamp\": \"invalid timestamp\",\n"
			       "    \"hostName\":  \"www.example.com\",\n"
			       "    \"content\":   \"Error!\"\n"
			       "  }\n"
			       "}\n");
	}

	void test_noHostname(void)
	{
		assertValidate(gcut_take_new_list_string(
				       "$.body.hostName must exist",
				       NULL),
			       "{\n"
			       "  \"type\": \"event\"\n,"
			       "  \"body\": {\n"
			       "    \"id\":        1,\n"
			       "    \"timestamp\": 1407824772.9396641,\n"
			       "    \"content\":   \"Error!\"\n"
			       "  }\n"
			       "}\n");
	}

	void test_noContent(void)
	{
		assertValidate(gcut_take_new_list_string(
				       "$.body.content must exist",
				       NULL),
			       "{\n"
			       "  \"type\": \"event\"\n,"
			       "  \"body\": {\n"
			       "    \"id\":        1,\n"
			       "    \"timestamp\": 1407824772.9396641,\n"
			       "    \"hostName\":  \"www.example.com\"\n"
			       "  }\n"
			       "}\n");
	}
#undef assertValidate
}

namespace getters {
	JsonParser *parser;
	GateJSONEventMessage *message;

	void cut_setup(void)
	{
		parser = json_parser_new();
		message = NULL;

		message = parseRaw(parser,
				   "{\n"
				   "  \"type\": \"event\",\n"
				   "  \"body\": {\n"
				   "    \"id\":        1,\n"
				   "    \"timestamp\": 1407824772.939664125,\n"
				   "    \"hostName\":  \"www.example.com\",\n"
				   "    \"content\":   \"Error!\"\n"
				   "  }\n"
				   "}\n");
	}

	void cut_teardown(void)
	{
		delete message;
		g_object_unref(parser);
	}

	void test_getID()
	{
		cppcut_assert_equal(static_cast<int64_t>(1),
				    message->getID());
	}

	void test_getHostName()
	{
		cppcut_assert_equal("www.example.com", message->getHostName());
	}

	void test_getContent()
	{
		cppcut_assert_equal("Error!", message->getContent());
	}
}

namespace timestampGetter {
	JsonParser *parser;
	GateJSONEventMessage *message;

	void cut_setup(void)
	{
		parser = json_parser_new();
		message = NULL;
	}

	void cut_teardown(void)
	{
		delete message;
		g_object_unref(parser);
	}

	void test_double()
	{
		const char *json =
			"{\n"
			"  \"type\": \"event\",\n"
			"  \"body\": {\n"
			"    \"id\":        1,\n"
			"    \"timestamp\": 1407824772.939664125,\n"
			"    \"hostName\":  \"www.example.com\",\n"
			"    \"content\":   \"Error!\"\n"
			"  }\n"
			"}\n";
		message = parseRaw(parser, json);
		timespec timestamp = message->getTimestamp();
		cppcut_assert_equal(static_cast<time_t>(1407824772),
				    timestamp.tv_sec);
		cppcut_assert_equal(static_cast<long>(939664125),
				    timestamp.tv_nsec);
	}

	void test_iso8601String()
	{
		const char *json =
			"{\n"
			"  \"type\": \"event\",\n"
			"  \"body\": {\n"
			"    \"id\":        1,\n"
			"    \"timestamp\": \"2014-08-12T06:26:12.939664Z\",\n"
			"    \"hostName\":  \"www.example.com\",\n"
			"    \"content\":   \"Error!\"\n"
			"  }\n"
			"}\n";
		message = parseRaw(parser, json);
		timespec timestamp = message->getTimestamp();
		cppcut_assert_equal(static_cast<time_t>(1407824772),
				    timestamp.tv_sec);
		cppcut_assert_equal(static_cast<long>(939664000),
				    timestamp.tv_nsec);
	}
}

namespace severityGetter {
	JsonParser *parser;
	GateJSONEventMessage *message;

	void cut_setup(void)
	{
		parser = json_parser_new();
		message = NULL;
	}

	void cut_teardown(void)
	{
		delete message;
		g_object_unref(parser);
	}

	GateJSONEventMessage *parse(const string &severityValue)
	{
		string json;

		json += "{\n";
		json += "  \"type\": \"event\",\n";
		json += "  \"body\": {\n";
		if (!severityValue.empty()) {
			json += "\"severity\": \"" + severityValue + "\",\n";
		}
		json += "    \"id\":        1,\n";
		json += "    \"timestamp\": 1407824772.939664125,\n";
		json += "    \"hostName\":  \"www.example.com\",\n";
		json += "    \"content\":   \"Error!\"\n";
		json += "  }\n";
		json += "}\n";

		return parseRaw(parser, json);
	}

	void test_default()
	{
		message = parse("");
		cppcut_assert_equal(TRIGGER_SEVERITY_UNKNOWN,
				    message->getSeverity());
	}

	void test_unknown()
	{
		message = parse("unknown");
		cppcut_assert_equal(TRIGGER_SEVERITY_UNKNOWN,
				    message->getSeverity());
	}

	void test_info()
	{
		message = parse("info");
		cppcut_assert_equal(TRIGGER_SEVERITY_INFO,
				    message->getSeverity());
	}

	void test_warning()
	{
		message = parse("warning");
		cppcut_assert_equal(TRIGGER_SEVERITY_WARNING,
				    message->getSeverity());
	}

	void test_error()
	{
		message = parse("error");
		cppcut_assert_equal(TRIGGER_SEVERITY_ERROR,
				    message->getSeverity());
	}

	void test_critical()
	{
		message = parse("critical");
		cppcut_assert_equal(TRIGGER_SEVERITY_CRITICAL,
				    message->getSeverity());
	}

	void test_emergency()
	{
		message = parse("emergency");
		cppcut_assert_equal(TRIGGER_SEVERITY_EMERGENCY,
				    message->getSeverity());
	}
}

} // namespace testGateJSONEventMessage
