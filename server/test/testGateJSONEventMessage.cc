/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gcutter.h>
#include <cppcutter.h>

#include <GateJSONEventMessage.h>

using namespace std;
using namespace mlpl;

namespace testGateJSONEventMessage {

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
		GError *error = NULL;
		json_parser_load_from_data(parser,
					   json.c_str(),
					   json.length(),
					   &error);
		gcut_assert_error(error);

		JsonNode *root;
		root = json_parser_get_root(parser);
		message = new GateJSONEventMessage(root);
	}

	const GList *getErrorList()
	{
		StringListIterator it = errors->begin();
		GList *errorList = NULL;
		for (; it != errors->end(); ++it) {
			string &message = *it;
			const char *copiedMessage =
				cut_take_strdup(message.c_str());
			errorList =
				g_list_append(errorList,
					      const_cast<char *>(copiedMessage));
		}
		return gcut_take_list(errorList, NULL);
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

} // namespace testGateJSONEventMessage
