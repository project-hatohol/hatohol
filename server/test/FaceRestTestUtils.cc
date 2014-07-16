/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <cppcutter.h>
#include <MutexLock.h>
#include "Hatohol.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "JsonParserAgent.h"
#include "DBClientTest.h"
#include "Params.h"
#include "MultiLangTest.h"
#include "CacheServiceDBClient.h"
#include "UnifiedDataStore.h"
#include "ZabbixAPIEmulator.h"
#include "SessionManager.h"
#include "testDBClientHatohol.h"
#include "HatoholArmPluginInterface.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

static const unsigned int TEST_PORT = 53194;
static const char *TEST_DB_HATOHOL_NAME = "testDatabase-hatohol.db";

static StringMap    emptyStringMap;
static StringVector emptyStringVector;
static FaceRest *g_faceRest = NULL;

void setupUserDB(void)
{
	const bool dbRecreate = true;
	// If loadTestDat is true, not only the user DB but also
	// the access info DB will be loaded. So we set false here
	// and load the user DB later.
	const bool loadTestDat = false;
	setupTestDBUser(dbRecreate, loadTestDat);
	loadTestDBUser();
}

void startFaceRest(void)
{
	struct : public FaceRestParam {
		MutexLock mutex;
		virtual void setupDoneNotifyFunc(void) 
		{
			mutex.unlock();
		}
	} param;

	string dbPathHatohol  = getFixturesDir() + TEST_DB_HATOHOL_NAME;
	bool dbRecreate = true, loadTestData = true;
	setupTestDBConfig(dbRecreate, loadTestData);

	defineDBPath(DB_DOMAIN_ID_HATOHOL, dbPathHatohol);

	CommandLineArg arg;
	arg.push_back("--face-rest-port");
	arg.push_back(StringUtils::sprintf("%u", TEST_PORT));
	g_faceRest = new FaceRest(arg, &param);
	g_faceRest->setNumberOfPreLoadWorkers(1);

	param.mutex.lock();
	g_faceRest->start();

	// wait for the setup completion of FaceReset
	param.mutex.lock();
}

void stopFaceRest(void)
{
	if (g_faceRest) {
		try {
			g_faceRest->waitExit();
		} catch (const HatoholException &e) {
			printf("Got exception: %s\n",
			       e.getFancyMessage().c_str());
		}
		delete g_faceRest;
		g_faceRest = NULL;
	}
}

string makeSessionIdHeader(const string &sessionId)
{
	string header =
	  StringUtils::sprintf("%s:%s", FaceRest::SESSION_ID_HEADER_NAME,
	                                sessionId.c_str());
	return header;
}

static string makeQueryString(const StringMap &parameters,
                              const string &callbackName)
{
	GHashTable *hashTable = g_hash_table_new(g_str_hash, g_str_equal);
	cppcut_assert_not_null(hashTable);
	StringMapConstIterator it = parameters.begin();
	for (; it != parameters.end(); ++it) {
		string key = it->first;
		string val = it->second;
		g_hash_table_insert(hashTable,
		                    (void *)key.c_str(), (void *)val.c_str());
	}
	if (!callbackName.empty()) {
		g_hash_table_insert(hashTable, (void *)"callback",
		                    (void *)callbackName.c_str());
		g_hash_table_insert(hashTable, (void *)"fmt", (void *)"jsonp");
	}

	char *encoded = soup_form_encode_hash(hashTable);
	string queryString = encoded;
	g_free(encoded);
	g_hash_table_unref(hashTable);
	return queryString;
}

static string makeQueryStringForCurlPost(const StringMap &parameters,
                                         const string &callbackName)
{
	string postDataArg;
	StringVector queryVect;
	string joinedString = makeQueryString(parameters, callbackName);
	StringUtils::split(queryVect, joinedString, '&');
	for (size_t i = 0; i < queryVect.size(); i++) {
		postDataArg += " -d ";
		postDataArg += queryVect[i];
	}
	return postDataArg;
}

static void getSessionId(RequestArg &arg)
{
	if (arg.userId == INVALID_USER_ID)
		return;
	SessionManager *sessionMgr = SessionManager::getInstance();
	const string sessionId = sessionMgr->create(arg.userId);
	arg.headers.push_back(makeSessionIdHeader(sessionId));
}

static bool parseStatusLine(RequestArg &arg, const gchar *line)
{
	bool succeeded = false;
	gchar **fields = g_strsplit(line, " ", -1);
	cut_take_string_array(fields);
	for (int i = 0; fields[i]; i++) {
		string field = fields[i];
		if (i == 0 && field != "HTTP/1.1")
			break;
		if (i == 1)
			arg.httpStatusCode = atoi(field.c_str());
		if (i == 2) {
			arg.httpReasonPhrase = field;
			succeeded = true;
		}
	}
	return succeeded;
}

void getServerResponse(RequestArg &arg)
{
	getSessionId(arg);

	// make encoded query parameters
	string joinedQueryParams;
	string postDataArg;
	if (arg.request == "POST" || arg.request == "PUT") {
		postDataArg =
		   makeQueryStringForCurlPost(arg.parameters, arg.callbackName);
	} else {
		joinedQueryParams = makeQueryString(arg.parameters,
		                                    arg.callbackName);
		if (!joinedQueryParams.empty())
			joinedQueryParams = "?" + joinedQueryParams;
	}

	// headers
	string headers;
	for (size_t i = 0; i < arg.headers.size(); i++) {
		headers += "-H \"";
		headers += arg.headers[i];
		headers += "\" ";
	}

	// get reply with wget
	string getCmd =
	  StringUtils::sprintf("curl -X %s %s %s -i http://localhost:%u%s%s",
	                       arg.request.c_str(), headers.c_str(),
	                       postDataArg.c_str(), TEST_PORT, arg.url.c_str(),
	                       joinedQueryParams.c_str());
	arg.response = executeCommand(getCmd);
	const string separator = "\r\n\r\n";
	size_t pos = arg.response.find(separator);
	if (pos != string::npos) {
		string headers = arg.response.substr(0, pos);
		arg.response = arg.response.substr(pos + separator.size());
		gchar **tmp = g_strsplit(headers.c_str(), "\r\n", -1);
		cut_take_string_array(tmp);
		for (size_t i = 0; tmp[i]; i++) {
			if (i == 0)
				cut_assert_true(parseStatusLine(arg, tmp[i]));
			else
				arg.responseHeaders.push_back(tmp[i]);
		}
	}
}

JsonParserAgent *getResponseAsJsonParser(RequestArg &arg)
{
	getServerResponse(arg);
	cut_assert_true(SOUP_STATUS_IS_SUCCESSFUL(arg.httpStatusCode));

	// if JSONP, check the callback name
	if (!arg.callbackName.empty()) {
		size_t lenCallbackName = arg.callbackName.size();
		size_t minimumLen = lenCallbackName + 2; // +2 for ''(' and ')'
		cppcut_assert_equal(true, arg.response.size() > minimumLen,
		  cut_message("length: %zd, minmumLen: %zd\n%s",
		              arg.response.size(), minimumLen,
		              arg.response.c_str()));

		cut_assert_equal_substring(
		  arg.callbackName.c_str(), arg.response.c_str(),
		  lenCallbackName);
		cppcut_assert_equal(')', arg.response[arg.response.size()-1]);
		arg.response = string(arg.response, lenCallbackName+1,
		                      arg.response.size() - minimumLen);
	}

	// check the JSON body
	if (isVerboseMode())
		cut_notify("<<response>>\n%s\n", arg.response.c_str());
	JsonParserAgent *parser = new JsonParserAgent(arg.response);
	if (parser->hasError()) {
		string parserErrMsg = parser->getErrorMessage();
		delete parser;
		cut_fail("%s\n%s", parserErrMsg.c_str(), arg.response.c_str());
	}
	return parser;
}

void _assertValueInParser(JsonParserAgent *parser,
			  const string &member, const bool expected)
{
	bool val;
	cppcut_assert_equal(true, parser->read(member, val),
	                    cut_message("member: %s, expect: %d",
	                                member.c_str(), expected));
	cppcut_assert_equal(expected, val);
}

void _assertValueInParser(JsonParserAgent *parser,
			  const string &member, uint32_t expected)
{
	int64_t val;
	cppcut_assert_equal(true, parser->read(member, val),
	                    cut_message("member: %s, expect: %" PRIu32,
	                                member.c_str(), expected));
	cppcut_assert_equal(expected, (uint32_t)val);
}

void _assertValueInParser(JsonParserAgent *parser,
			  const string &member, int expected)
{
	int64_t val;
	cppcut_assert_equal(true, parser->read(member, val),
	                    cut_message("member: %s, expect: %d",
	                                member.c_str(), expected));
	cppcut_assert_equal(expected, (int)val);
}

void _assertValueInParser(JsonParserAgent *parser,
			  const string &member, uint64_t expected)
{
	int64_t val;
	cppcut_assert_equal(true, parser->read(member, val));
	cppcut_assert_equal(expected, (uint64_t)val);
}

void _assertValueInParser(JsonParserAgent *parser,
			  const string &member, const timespec &expected)
{
	int64_t val;
	cppcut_assert_equal(true, parser->read(member, val));
	cppcut_assert_equal((int64_t)expected.tv_sec, val);
}

void _assertValueInParser(JsonParserAgent *parser,
			  const string &member, const string &expected)
{
	string val;
	cppcut_assert_equal(true, parser->read(member, val));
	cppcut_assert_equal(expected, val);
}

void _assertStartObject(JsonParserAgent *parser, const string &keyName)
{
	cppcut_assert_equal(true, parser->startObject(keyName),
	                    cut_message("Key: '%s'", keyName.c_str()));
}

void _assertNoValueInParser(
  JsonParserAgent *parser, const string &member)
{
	string val;
	cppcut_assert_equal(false, parser->read(member, val));
}

void _assertNullInParser(JsonParserAgent *parser, const string &member)
{
	bool result = false;
	cppcut_assert_equal(true, parser->isNull(member, result));
	cppcut_assert_equal(true, result);
}

void _assertErrorCode(JsonParserAgent *parser,
		      const HatoholErrorCode &expectCode)
{
	assertValueInParser(parser, "apiVersion", FaceRest::API_VERSION);
	assertValueInParser(parser, "errorCode", expectCode);
}
