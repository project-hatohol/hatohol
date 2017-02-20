/*
 * Copyright (C) 2013-2016 Project Hatohol
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

#include <cppcutter.h>
#include <gcutter.h>
#include <unistd.h>
#include <typeinfo>
#include <errno.h>
#include <JSONParser.h>
#include <SeparatorInjector.h>
#include "Helpers.h"
#include "DBTablesConfig.h"
#include "DBTablesAction.h"
#include "DBTablesTest.h"
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"
#include "ThreadLocalDBCache.h"
#include "SQLUtils.h"
#include "Reaper.h"
#include "ConfigManager.h"
#include "HatoholDBUtils.h"

using namespace std;
using namespace mlpl;

void _assertStringVector(const StringVector &expected,
                         const StringVector &actual)
{
	cppcut_assert_equal(expected.size(), actual.size());
	for (size_t i = 0; i < expected.size(); i++)
		cppcut_assert_equal(expected[i], actual[i]);
}

void _assertStringVectorVA(const StringVector *actual, ...)
{
	StringVector expectedVect;
	va_list valist;
	va_start(valist, actual);
	while (true) {
		const char *expected = va_arg(valist, const char *);
		if (!expected)
			break;
		expectedVect.push_back(expected);
	}
	va_end(valist);
	assertStringVector(expectedVect, *actual);
}

void _assertExist(const string &target, const string &words)
{
	StringVector splitWords;
	string wordsStripCR = StringUtils::eraseChars(words, "\n");
	StringUtils::split(splitWords, wordsStripCR, ' ');
	for (size_t i = 0; i < splitWords.size(); i++) {
		if (splitWords[i] == target)
			return;
	}
	cut_fail("Not found: %s in %s", target.c_str(), words.c_str());
}

void _assertItemTable(const ItemTablePtr &expect, const ItemTablePtr &actual)
{
	const ItemGroupList &expectList = expect->getItemGroupList();
	const ItemGroupList &actualList = actual->getItemGroupList();
	cppcut_assert_equal(expectList.size(), actualList.size());
	ItemGroupListConstIterator exItr = expectList.begin();
	ItemGroupListConstIterator acItr = actualList.begin();

	size_t grpCnt = 0;
	for (; exItr != expectList.end(); ++exItr, ++acItr, grpCnt++) {
		const ItemGroup *expectGroup = *exItr;
		const ItemGroup *actualGroup = *acItr;
		size_t numberOfItems = expectGroup->getNumberOfItems();
		cppcut_assert_equal(numberOfItems,
		                    actualGroup->getNumberOfItems());
		for (size_t index = 0; index < numberOfItems; index++){
			const ItemData *expectData =
			   expectGroup->getItemAt(index);
			const ItemData *actualData =
			   actualGroup->getItemAt(index);
			cppcut_assert_equal(
			  *expectData, *actualData,
			  cut_message("index: %zd, group count: %zd",
			              index, grpCnt));
		}
	}
}

// TODO: We should prepare '== operator' and cppcut_assert_equal() ?
void _assertEqual(const std::set<int> &expect, const std::set<int> &actual)
{
	assertEqualSet(expect, actual);
}

// TODO: Use assertEqualSet<string>()
void _assertEqual(const set<string> &expect, const set<string> &actual)
{
	auto errMsg = [&] {
		struct {
			string title;
			const set<string> &members;
		} args[] = {
			{"<expect>", expect}, {"<actual>", actual}
		};

		string s;
		for (auto a: args) {
			s += a.title + "\n";
			for (auto val: a.members)
				s += val + "\n";
		}
		return s;
	};

	cppcut_assert_equal(expect.size(), actual.size(),
	                    cut_message("%s\n", errMsg().c_str()));
	for (auto val : expect) {
		cppcut_assert_equal(
		  true, actual.find(val) != actual.end(),
		  cut_message("Not found: %s", val.c_str()));
	}
}

void _assertEqual(
  const MonitoringServerInfo &expect, const MonitoringServerInfo &actual)
{
	cppcut_assert_equal(expect.id,        actual.id);
	cppcut_assert_equal(expect.type,      actual.type);
	cppcut_assert_equal(expect.hostName,  actual.hostName);
	cppcut_assert_equal(expect.ipAddress, actual.ipAddress);
	cppcut_assert_equal(expect.nickname,  actual.nickname);
	cppcut_assert_equal(expect.port,      actual.port);
	cppcut_assert_equal(expect.pollingIntervalSec, actual.pollingIntervalSec);
	cppcut_assert_equal(expect.retryIntervalSec, actual.retryIntervalSec);
	cppcut_assert_equal(expect.userName,  actual.userName);
	cppcut_assert_equal(expect.password,  actual.password);
	cppcut_assert_equal(expect.dbName,    actual.dbName);
}

void _assertEqual(const ArmInfo &expect, const ArmInfo &actual)
{
	cppcut_assert_equal(expect.running,    actual.running);
	cppcut_assert_equal(expect.stat,       actual.stat);
	cppcut_assert_equal(expect.statUpdateTime, actual.statUpdateTime);
	cppcut_assert_equal(expect.failureComment, actual.failureComment);
	cppcut_assert_equal(expect.lastSuccessTime, actual.lastSuccessTime);
	cppcut_assert_equal(expect.lastFailureTime, actual.lastFailureTime);
	cppcut_assert_equal(expect.numUpdate,  actual.numUpdate);
	cppcut_assert_equal(expect.numFailure, actual.numFailure);
}

void _assertEqual(const ItemInfo &expect, const ItemInfo &actual)
{
	cppcut_assert_equal(expect.serverId,  actual.serverId);
	cppcut_assert_equal(expect.id,        actual.id);
	cppcut_assert_equal(expect.globalHostId,   actual.globalHostId);
	cppcut_assert_equal(expect.hostIdInServer, actual.hostIdInServer);
	cppcut_assert_equal(expect.brief,     actual.brief);
	cppcut_assert_equal(expect.lastValueTime.tv_sec,
	                    actual.lastValueTime.tv_sec);
	cppcut_assert_equal(expect.lastValueTime.tv_nsec,
	                    actual.lastValueTime.tv_nsec);
	cppcut_assert_equal(expect.lastValue, actual.lastValue);
	cppcut_assert_equal(expect.prevValue, actual.prevValue);
	assertStringVector(expect.categoryNames, actual.categoryNames);
	cppcut_assert_equal(expect.delay,     actual.delay);
	cppcut_assert_equal(expect.valueType, actual.valueType);
	cppcut_assert_equal(expect.unit,      actual.unit);
}

void _assertEqual(const TriggerInfo &expect, const TriggerInfo &actual)
{
	cppcut_assert_equal(expect.serverId,     actual.serverId);
	cppcut_assert_equal(expect.id,           actual.id);
	cppcut_assert_equal(expect.status,       actual.status);
	cppcut_assert_equal(expect.severity,     actual.severity);
	cppcut_assert_equal(expect.lastChangeTime.tv_sec,
	                    actual.lastChangeTime.tv_sec);
	cppcut_assert_equal(expect.lastChangeTime.tv_nsec,
	                    actual.lastChangeTime.tv_nsec);
	cppcut_assert_equal(expect.globalHostId,   actual.globalHostId);
	cppcut_assert_equal(expect.hostIdInServer, actual.hostIdInServer);
	cppcut_assert_equal(expect.hostName,     actual.hostName);
	cppcut_assert_equal(expect.brief,        actual.brief);
	cppcut_assert_equal(TRIGGER_VALID,       actual.validity);
}

// This can also be used as
//     _assertEqual(const ServerHostSetMap &expect,
//                  const ServerHostSetMap &actual)
// because the actual (primitive level) definition is the indentical.
void _assertEqual(const ServerHostGrpSetMap &expect,
                  const ServerHostGrpSetMap &actual)
{
	cppcut_assert_equal(expect.size(), actual.size());
	for (const auto &elem: expect) {
		const auto &key = elem.first;
		assertHas(actual, key);
		const auto &actual_elem = actual.find(key)->second;
		assertEqual(elem.second, actual_elem);
	}
}

template <typename T>
void assertJSONParser(JSONParser &expectParser, JSONParser &actualParser,
                      const string &member)
{
	T actual;
	T expect;
	cppcut_assert_equal(true, expectParser.read(member, expect));
	cppcut_assert_equal(true, actualParser.read(member, actual));
	cppcut_assert_equal(expect, actual,
	                    cut_message("member: %s", member.c_str()));
}

extern void _assertEqualJSONString(const string &expect, const string &actual)
{
	JSONParser expectParser(expect);
	JSONParser actualParser(actual);

	map<JSONParser::ValueType, function<void(const string &)>> assertMap;
	function<void()> assertObject = [&]() {
		set<string> expectMembers;
		set<string> actualMembers;
		expectParser.getMemberNames(expectMembers);
		actualParser.getMemberNames(actualMembers);
		assertEqual(expectMembers, actualMembers);
		for (auto name : expectMembers) {
			auto expectType = expectParser.getValueType(name);
			auto actualType = actualParser.getValueType(name);
			cppcut_assert_equal(expectType, actualType);
			auto assertionFuncItr = assertMap.find(expectType);
			if (assertionFuncItr == assertMap.end()) {
				cut_fail("Unknown type: %d (%s)",
				         expectType, name.c_str());
			}
			(assertionFuncItr->second)(name);
		}
	};

	auto startBothObjects = [&](const string &name) {
		expectParser.startObject(name);
		actualParser.startObject(name);
	};
	auto endBothObjects = [&]() {
		expectParser.endObject();
		actualParser.endObject();
	};
	assertMap[JSONParser::VALUE_TYPE_NULL] = [&](const string &name){
	};
	assertMap[JSONParser::VALUE_TYPE_BOOLEAN] = [&](const string &name) {
		assertJSONParser<bool>(expectParser, actualParser, name);
	};
	assertMap[JSONParser::VALUE_TYPE_INT64] = [&](const string &name) {
		assertJSONParser<int64_t>(expectParser, actualParser, name);
	};
	assertMap[JSONParser::VALUE_TYPE_DOUBLE] = [&](const string &name) {
		assertJSONParser<double>(expectParser, actualParser, name);
	};
	assertMap[JSONParser::VALUE_TYPE_STRING] = [&](const string &name) {
		assertJSONParser<string>(expectParser, actualParser, name);
	};
	assertMap[JSONParser::VALUE_TYPE_OBJECT] = [&](const string &name) {
		startBothObjects(name);
		assertObject();
		endBothObjects();
	};
	assertMap[JSONParser::VALUE_TYPE_ARRAY] = [&](const string &name) {
		startBothObjects(name);
		const unsigned int size = expectParser.countElements();
		cppcut_assert_equal(size, actualParser.countElements());
		for (unsigned int i = 0; i < size; i++) {
			expectParser.startElement(i);
			actualParser.startElement(i);
			assertObject();
			expectParser.endElement();
			actualParser.endElement();
		}
		endBothObjects();
	};

	assertObject();
}

struct SpawnSyncContext {
	bool running;
	bool hasError;
	GIOChannel *ioch;
	guint dataCbId;
	guint errCbId;
	string &msg;

	SpawnSyncContext(int fd, string &_msg)
	: running(true),
	  ioch(NULL),
	  msg(_msg)
	{
		ioch = g_io_channel_unix_new(fd);
		cppcut_assert_not_null(ioch);

		GError *error = NULL;
		g_io_channel_set_encoding(ioch, NULL, &error);
		gcut_assert_error(error);

		// non blocking
		GIOFlags flags = G_IO_FLAG_NONBLOCK;
		g_io_channel_set_flags(ioch, flags, &error);
		gcut_assert_error(error);

		// data callback
		GIOCondition cond = G_IO_IN;
		dataCbId = g_io_add_watch(ioch, cond, cbData, this);

		// error callback
		cond = (GIOCondition)(G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL);
		errCbId = g_io_add_watch(ioch, cond, cbErr, this);
	}

	virtual ~SpawnSyncContext()
	{
		Utils::removeEventSourceIfNeeded(dataCbId);
		Utils::removeEventSourceIfNeeded(errCbId);
		g_io_channel_unref(ioch);
	}

	static gboolean cbData(GIOChannel *source, GIOCondition condition,
	                gpointer data)
	{
		SpawnSyncContext *obj = static_cast<SpawnSyncContext *>(data);
		GError *error = NULL;
		gsize bytesRead;
		gsize bufSize = 0x1000;
		gchar buf[bufSize];
		do {
			GIOStatus stat =
			  g_io_channel_read_chars(source,
						  buf, bufSize-1,
						  &bytesRead,
						  &error);
			if (stat != G_IO_STATUS_NORMAL) {
				MLPL_ERR("Failed to call g_io_channel_read_chars: %s\n",
					 error ? error->message : "unknown");
				if (error)
					g_error_free(error);
				obj->running = false;
				obj->dataCbId = INVALID_EVENT_ID;
				return FALSE;
			}
			buf[bytesRead] = '\0';
			obj->msg += buf;
		} while(bytesRead == bufSize - 1);
		return TRUE;
	}

	static gboolean cbErr(GIOChannel *source, GIOCondition condition,
	                gpointer data)
	{
		SpawnSyncContext *obj = static_cast<SpawnSyncContext *>(data);
		obj->running = false;
		obj->errCbId = INVALID_EVENT_ID;
		return FALSE;
	}
};

gboolean spawnSync(const string &commandLine, string &stdOut, string &stdErr,
                   GError **error)
{
	gchar **argv = NULL;
	if (!g_shell_parse_argv(commandLine.c_str(), NULL, &argv, error))
		return FALSE;

	const gchar *workingDir = NULL;
	gchar **envp = NULL;
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	GSpawnChildSetupFunc childSetup = NULL;
	gpointer userData = NULL;
	GPid childPid;
	gint stdOutFd = -1;
	gint stdErrFd = -1;;
	gboolean ret =
	  g_spawn_async_with_pipes(workingDir, argv, envp, flags,
	                           childSetup, userData, &childPid,
	                           NULL /* stdInFd */, &stdOutFd, &stdErrFd,
	                           error);
	g_strfreev(argv);
	if (!ret)
		return FALSE;

	SpawnSyncContext ctxStd(stdOutFd, stdOut);
	SpawnSyncContext ctxErr(stdErrFd, stdErr);

	while (ctxStd.running && ctxErr.running)
		g_main_context_iteration(NULL, TRUE);

	close(stdOutFd);
	close(stdErrFd);
	return TRUE;
}

string executeCommand(const string &commandLine)
{
	gboolean ret;
	string errorStr;
	string stdoutStr, stderrStr;
	GError *error = NULL;

	// g_spawn_sync families cannot be used in the Hatohol's test.
	// Because they update the signal handler for SIGCHLD. As a result,
	// Hatohol's SIGCHLD signal handler is ignored.
	ret = spawnSync(commandLine, stdoutStr, stderrStr, &error);
	if (!ret)
		goto err;
	return stdoutStr;

err:
	if (error) {
		errorStr = error->message;
		g_error_free(error);
	}
	cut_fail("<<stdout>>\n%s\n<<stderr>>\n%s\n<<error->message>>\n%s",
	         stdoutStr.c_str(), stderrStr.c_str(), errorStr.c_str());
	return "";
}

string getBaseDir(void)
{
	const gchar *dir;
	dir = g_getenv("BASE_DIR");
	return dir ? dir : ".";
}

string getFixturesDir(void)
{
	string dir = getBaseDir();
	dir += G_DIR_SEPARATOR;
	dir += "fixtures";
	dir += G_DIR_SEPARATOR;
	return dir;
}

bool isVerboseMode(void)
{
	static bool checked = false;
	static bool verboseMode = false;
	if (!checked) {
		char *env = getenv("VERBOSE");
		if (env && env == string("1"))
			verboseMode = true;
		checked = true;
	}
	return verboseMode;
}

void notifyIfVerboseMode(const string &s0, const string &s1)
{
	if (!isVerboseMode())
		return;
	cut_notify("<s0> %s\n<s1> %s\n", s0.c_str(), s1.c_str());
}

// TODO: consider this method is still needed ?
string getDBPathForDBClientHatohol(void)
{
	// TODO: Should we get the DB name from DBHatohol, although such API
	//       hasn't been implemented yet.
	static const string DEFAULT_DB_NAME = "hatohol";

	struct callgate : public DBTablesMonitoring, public DBAgentSQLite3 {
		static string getDBPath(void) {
			return makeDBPathFromName(DEFAULT_DB_NAME);
		}
	};
	return callgate::getDBPath();
}

string execSqlite3ForDBClient(const string &dbPath, const string &statement)
{
	cut_assert_exist_path(dbPath.c_str());
	string commandLine =
	  StringUtils::sprintf("sqlite3 %s \"%s\"",
	                       dbPath.c_str(), statement.c_str());
	string result = executeCommand(commandLine);
	return result;
}

string execMySQL(const string &dbName, const string &statement, bool showHeader)
{
	const char *headerOpt = showHeader ? "" : "-N";
	//
	// We use a --raw option that disable the escape such as \ -> \\.
	//
	string commandLine =
	  StringUtils::sprintf("mysql --raw -B %s -D %s -e \"%s\"",
	                       headerOpt, dbName.c_str(), statement.c_str());
	string result = executeCommand(commandLine);
	return result;
}

void _assertDatetime(int expectedClock, int actualClock)
{
	if (expectedClock == CURR_DATETIME)
		assertCurrDatetime(actualClock);
	else
		cppcut_assert_equal(expectedClock, actualClock);
}

void _assertCurrDatetime(int clock)
{
	// On new TravisCI infrastructure since Dec.2015,
	// some tests need approx. 5 sec for the completion of the DB commit.
	// We set here the timeout value that is several times of it.
	const int MAX_ALLOWD_CURR_TIME_ERROR = 15;
	int currClock = (int)time(NULL);
	cppcut_assert_equal(
	  true, currClock >= clock,
	  cut_message("currClock: %d, clock: %d", currClock, clock));
	cppcut_assert_equal(
	  true, currClock - clock < MAX_ALLOWD_CURR_TIME_ERROR,
	  cut_message( "currClock: %d, clock: %d", currClock, clock));
}

void _assertCurrDatetime(const string &datetime)
{
	ItemDataPtr item = SQLUtils::createFromString(datetime.c_str(),
	                                              SQL_COLUMN_TYPE_DATETIME);
	int clock = *item;
	assertCurrDatetime(clock);
}

string getExpectedNullNotation(DBAgent &dbAgent)
{
	const type_info &tid = typeid(dbAgent);
	if (tid == typeid(DBAgentMySQL))
		return "NULL";
	else if (tid == typeid(DBAgentSQLite3))
		return "";
	else
		cut_fail("Unknown type: %s", DEMANGLED_TYPE_NAME(dbAgent));
	return "";
}

string makeServerInfoOutput(const MonitoringServerInfo &serverInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%s|%s|%s|%d|%d|%d|%s|%s|%s|%s|%s\n",
	                        serverInfo.id, serverInfo.type,
	                        serverInfo.hostName.c_str(),
	                        serverInfo.ipAddress.c_str(),
	                        serverInfo.nickname.c_str(),
	                        serverInfo.port,
	                        serverInfo.pollingIntervalSec,
	                        serverInfo.retryIntervalSec,
	                        serverInfo.userName.c_str(),
	                        serverInfo.password.c_str(),
	                        serverInfo.dbName.c_str(),
	                        serverInfo.baseURL.c_str(),
				serverInfo.extendedInfo.c_str());
	return expectedOut;
}

std::string makeArmPluginInfoOutput(const ArmPluginInfo &armPluginInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%s|%s|%s|%" FMT_SERVER_ID "|"
				"%s|%s|%s|%d|%s\n",
	                        armPluginInfo.id, armPluginInfo.type,
	                        armPluginInfo.path.c_str(),
	                        armPluginInfo.brokerUrl.c_str(),
	                        armPluginInfo.staticQueueAddress.c_str(),
	                        armPluginInfo.serverId,
				armPluginInfo.tlsCertificatePath.c_str(),
				armPluginInfo.tlsKeyPath.c_str(),
				armPluginInfo.tlsCACertificatePath.c_str(),
				armPluginInfo.tlsEnableVerify,
				armPluginInfo.uuid.c_str());
	return expectedOut;
}

string makeIncidentTrackerInfoOutput(const IncidentTrackerInfo &incidentTrackerInfo)
{
	string expectedOut =
	  mlpl::StringUtils::sprintf(
	    "%" FMT_INCIDENT_TRACKER_ID "|%d|%s|%s|%s|%s|%s|%s\n",
	    incidentTrackerInfo.id,
	    incidentTrackerInfo.type,
	    incidentTrackerInfo.nickname.c_str(),
	    incidentTrackerInfo.baseURL.c_str(),
	    incidentTrackerInfo.projectId.c_str(),
	    incidentTrackerInfo.trackerId.c_str(),
	    incidentTrackerInfo.userName.c_str(),
	    incidentTrackerInfo.password.c_str());
	return expectedOut;
}

std::string makeUserRoleInfoOutput(const UserRoleInfo &userRoleInfo)
{
	return StringUtils::sprintf(
		 "%" FMT_USER_ROLE_ID "|%s|%" FMT_OPPRVLG "\n",
		 userRoleInfo.id, userRoleInfo.name.c_str(),
		 userRoleInfo.flags);
}

string makeTriggerOutput(const TriggerInfo &triggerInfo)
{
	string expectedOut =
	  StringUtils::sprintf(
	    "%" FMT_SERVER_ID "|%" FMT_TRIGGER_ID "|%d|%d|%ld|%lu|"
	    "%" FMT_HOST_ID "|%" FMT_LOCAL_HOST_ID "|%s|%s|%s|%d\n",
	    triggerInfo.serverId,
	    triggerInfo.id.c_str(),
	    triggerInfo.status, triggerInfo.severity,
	    triggerInfo.lastChangeTime.tv_sec,
	    triggerInfo.lastChangeTime.tv_nsec,
	    triggerInfo.globalHostId,
	    triggerInfo.hostIdInServer.c_str(),
	    triggerInfo.hostName.c_str(),
	    triggerInfo.brief.c_str(),
	    triggerInfo.extendedInfo.c_str(),
	    triggerInfo.validity);
	return expectedOut;
}

string makeEventOutput(const EventInfo &eventInfo)
{
	string output =
	  mlpl::StringUtils::sprintf(
	    "%" FMT_SERVER_ID "|%" FMT_EVENT_ID "|%ld|%ld|%d|%" FMT_TRIGGER_ID
	    "|%d|%u|%" FMT_HOST_ID "|%" FMT_LOCAL_HOST_ID "|%s|%s|%s\n",
	    eventInfo.serverId, eventInfo.id.c_str(),
	    eventInfo.time.tv_sec, eventInfo.time.tv_nsec,
	    eventInfo.type, eventInfo.triggerId.c_str(),
	    eventInfo.status, eventInfo.severity,
	    eventInfo.globalHostId,
	    eventInfo.hostIdInServer.c_str(),
	    eventInfo.hostName.c_str(),
	    eventInfo.brief.c_str(),
	    eventInfo.extendedInfo.c_str());
	return output;
}

string makeIncidentOutput(const IncidentInfo &incidentInfo)
{
	string output =
	  mlpl::StringUtils::sprintf(
	    "%" FMT_INCIDENT_TRACKER_ID "|%" FMT_SERVER_ID "|%" FMT_EVENT_ID
	    "|%" FMT_TRIGGER_ID "|%s|%s|%s|%s"
	    "|%" PRIu64 "|%" PRIu64 "|%" PRIu64 "|%" PRIu64
	    "|%s|%d|%" PRIu64 "|%d\n",
	    incidentInfo.trackerId,
	    incidentInfo.serverId,
	    incidentInfo.eventId.c_str(),
	    incidentInfo.triggerId.c_str(),
	    incidentInfo.identifier.c_str(),
	    incidentInfo.location.c_str(),
	    incidentInfo.status.c_str(),
	    incidentInfo.assignee.c_str(),
	    incidentInfo.createdAt.tv_sec,
	    incidentInfo.createdAt.tv_nsec,
	    incidentInfo.updatedAt.tv_sec,
	    incidentInfo.updatedAt.tv_nsec,
	    incidentInfo.priority.c_str(),
	    incidentInfo.doneRatio,
	    incidentInfo.unifiedEventId,
	    incidentInfo.commentCount);
	return output;
}

string makeHistoryOutput(const HistoryInfo &historyInfo)
{
	string output =
	  mlpl::StringUtils::sprintf(
	    "%" FMT_SERVER_ID "|%" FMT_ITEM_ID
	    "|%" PRIu64 "|%" PRIu64
	    "|%s\n",
	    historyInfo.serverId, historyInfo.itemId.c_str(),
	    (uint64_t)historyInfo.clock.tv_sec,
	    (uint64_t)historyInfo.clock.tv_nsec,
	    historyInfo.value.c_str());
	return output;
}

string makeItemOutput(const ItemInfo &itemInfo)
{
	string expectedOut =
	  StringUtils::sprintf(
	    "%" FMT_GEN_ID "|%" FMT_SERVER_ID "|%" FMT_ITEM_ID "|%" FMT_HOST_ID
	    "|%" FMT_LOCAL_HOST_ID "|%s|%ld|%lu|%s|%s|%d|%s",
	    itemInfo.globalId, itemInfo.serverId, itemInfo.id.c_str(),
	    itemInfo.globalHostId,
	    itemInfo.hostIdInServer.c_str(),
	    itemInfo.brief.c_str(),
	    itemInfo.lastValueTime.tv_sec,
	    itemInfo.lastValueTime.tv_nsec,
	    itemInfo.lastValue.c_str(),
	    itemInfo.prevValue.c_str(),
	    static_cast<int>(itemInfo.valueType),
	    itemInfo.unit.c_str());

	expectedOut += "\n";
	return expectedOut;
}

string makeHostsOutput(const ServerHostDef &svHostDef, const size_t &id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|" DBCONTENT_MAGIC_ANY "|%" FMT_SERVER_ID "|%s|%s|%d\n",
	  id + 1, svHostDef.serverId,
	  svHostDef.hostIdInServer.c_str(), svHostDef.name.c_str(),
	  HOST_STAT_NORMAL);

	return expectedOut;
}

string makeHostgroupsOutput(const Hostgroup &hostgrp, const size_t &id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|%" FMT_SERVER_ID "|%s|%s\n",
	  id + 1, hostgrp.serverId,
	  hostgrp.idInServer.c_str(), hostgrp.name.c_str());
	return expectedOut;
}

string makeMapHostsHostgroupsOutput(
  const HostgroupMember &hostgrpMember, const size_t &id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|%" FMT_SERVER_ID "|%s|%s|%" FMT_HOST_ID "\n",
	  id + 1,
	  hostgrpMember.serverId,
	  hostgrpMember.hostIdInServer.c_str(),
	  hostgrpMember.hostgroupIdInServer.c_str(),
	  hostgrpMember.hostId);

	return expectedOut;
}

std::string makeSeverityRankInfoOutput(const SeverityRankInfo &severityRankInfo)
{
	return StringUtils::sprintf(
		 "%" FMT_SEVERITY_RANK_ID "|%d|%s|%s|%d\n",
		 severityRankInfo.id, severityRankInfo.status,
		 severityRankInfo.color.c_str(),
		 severityRankInfo.label.c_str(),
		 severityRankInfo.asImportant);
}

std::string makeCustomIncidentStatusOutput(const CustomIncidentStatus &customIncidentStatus)
{
	return StringUtils::sprintf(
		 "%" FMT_CUSTOM_INCIDENT_STATUS_ID "|%s|%s\n",
		 customIncidentStatus.id,
		 customIncidentStatus.code.c_str(),
		 customIncidentStatus.label.c_str());
}

std::string makeIncidentHistoryOutput(const IncidentHistory &incidentHistory)
{
	return StringUtils::sprintf(
		 "%" FMT_INCIDENT_HISTORY_ID "|%" FMT_UNIFIED_EVENT_ID
		 "|%" FMT_USER_ID "|%s|%s|%" PRIu64 "|%" PRIu64 "\n",
		 incidentHistory.id,
		 incidentHistory.unifiedEventId,
		 incidentHistory.userId,
		 incidentHistory.status.c_str(),
		 incidentHistory.comment.c_str(),
		 incidentHistory.createdAt.tv_sec,
		 incidentHistory.createdAt.tv_nsec);
}

static void assertDBContentForComponets(const string &expect,
                                        const string &actual,
                                        DBAgent *dbAgent)
{
	StringVector wordsExpect;
	StringVector wordsActual;
	bool doMerge = false;
	StringUtils::split(wordsExpect, expect, '|', doMerge);
	StringUtils::split(wordsActual, actual, '|', doMerge);
	cppcut_assert_equal(wordsExpect.size(), wordsActual.size(),
	                    cut_message("<<expect>>: %s\n<<actual>>: %s\n",
	                                expect.c_str(), actual.c_str()));
	for (size_t i = 0; i < wordsExpect.size(); i++) {
		if (wordsExpect[i] == DBCONTENT_MAGIC_CURR_DATETIME) {
			assertCurrDatetime(wordsActual[i]);
		} else if (wordsExpect[i] == DBCONTENT_MAGIC_CURR_TIME) {
			assertCurrDatetime(atoi(wordsActual[i].c_str()));
		} else if(wordsExpect[i] == DBCONTENT_MAGIC_ANY) {
			// just pass
		} else if(wordsExpect[i] == DBCONTENT_MAGIC_NULL) {
			cppcut_assert_equal(getExpectedNullNotation(*dbAgent),
			                    wordsActual[i]);
		} else {
			cppcut_assert_equal(wordsExpect[i], wordsActual[i],
			  cut_message(
			    "line no: %zd\n<<expect>>: %s\n<<actual>>: %s\n",
	                    i, expect.c_str(), actual.c_str()));
		}
	}
}

void _assertDBContent(DBAgent *dbAgent, const string &statement,
                      const string &expect)
{
	const string actual = execSQL(dbAgent, statement);
	StringVector linesExpect;
	StringVector linesActual;
	StringUtils::split(linesExpect, expect, '\n');
	StringUtils::split(linesActual, actual, '\n');
	cppcut_assert_equal(linesExpect.size(), linesActual.size(),
	  cut_message("<<expect>>\n%s\n\n<<actual>>\n%s\n",
	              expect.c_str(), actual.c_str()));
	for (size_t i = 0; i < linesExpect.size(); i++) {
		cut_trace(
		  assertDBContentForComponets(linesExpect[i], linesActual[i],
		                              dbAgent));
	}
}

void _assertCreateTable(DBAgent *dbAgent, const string &tableName)
{
	string output;
	const type_info &tid = typeid(*dbAgent);
	if (tid == typeid(DBAgentMySQL)) {
		DBAgentMySQL *dba = dynamic_cast<DBAgentMySQL *>(dbAgent);
		string dbName = dba->getDBName();
		const string statement = StringUtils::sprintf(
		  "show tables like '%s'", tableName.c_str());
		output = execMySQL(dbName, statement);
	} else if (tid == typeid(DBAgentSQLite3)) {
		DBAgentSQLite3 *dba = dynamic_cast<DBAgentSQLite3 *>(dbAgent);
		string command = "sqlite3 " + dba->getDBPath() + " \".table\"";
		output = executeCommand(command);
	} else {
		cut_fail("Unkown type: %s", DEMANGLED_TYPE_NAME(*dbAgent));
	}

	assertExist(tableName, output);
}

void _assertExistIndex(DBAgent &dbAgent, const std::string &tableName,
                       const std::string &indexName, const size_t &numColumns)
{
	string output;
	const type_info &tid = typeid(dbAgent);
	if (tid == typeid(DBAgentMySQL)) {
		DBAgentMySQL &dba = dynamic_cast<DBAgentMySQL &>(dbAgent);
		string dbName = dba.getDBName();
		const string statement = StringUtils::sprintf(
		  "SHOW INDEX FROM %s where Key_name='%s';",
		  tableName.c_str(), indexName.c_str());
		output = execMySQL(dbName, statement);
		StringVector lines;
		StringUtils::split(lines, output, '\n');
		cppcut_assert_equal(numColumns, lines.size());
	} else {
		cut_fail("Unkown type: %s", DEMANGLED_TYPE_NAME(dbAgent));
	}
}

void _assertDBTablesVersion(DBAgent &dbAgent, const DBTablesId &tablesId,
                            const int &dbVersion)
{
	string expect = StringUtils::sprintf("%d|%d", tablesId, dbVersion);
	string statement = StringUtils::sprintf(
	  "SELECT * FROM _tables_version where tables_id=%d", tablesId);
	assertDBContent(&dbAgent, statement, expect);
}

void _assertTimeIsNow(const SmartTime &smtime, double allowedError)
{
	SmartTime diff(SmartTime::INIT_CURR_TIME);
	diff -= smtime;
	cppcut_assert_equal(
	  true, diff.getAsSec() < allowedError,
	  cut_message("time: %e, diff: %e, allowedError: %e",
	              smtime.getAsSec(), diff.getAsSec(), allowedError));
}

void _assertHatoholError(const HatoholErrorCode &code,
                         const HatoholError &err)
{
	HatoholError expected(code);
	cppcut_assert_equal(expected.getCodeName(), err.getCodeName());
	cppcut_assert_equal(code, err.getCode());
}

void _assertServersInDB(const ServerIdSet &excludeServerIdSet)
{
	string statement = "select * from servers ";
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		// We must make a copy because the member will be changed.
		MonitoringServerInfo serverInfo = testServerInfo[i];
		ServerIdSetIterator it = excludeServerIdSet.find(serverInfo.id);
		if (it != excludeServerIdSet.end())
			continue;
		expect += makeServerInfoOutput(serverInfo);
	}
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getConfig().getDBAgent(), statement, expect);
}

void _assertArmPluginsInDB(const set<int> &excludeIdSet)
{
	string statement = "SELECT * FROM arm_plugins ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestArmPluginInfo; i++) {
		const int id = i + 1;
		// We must make a copy because the member will be changed.
		ArmPluginInfo armPluginInfo = getTestArmPluginInfo()[i];
		armPluginInfo.id = id;
		set<int>::const_iterator it = excludeIdSet.find(id);
		if (it != excludeIdSet.end())
			continue;
		expect += makeArmPluginInfoOutput(armPluginInfo);
	}
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getConfig().getDBAgent(), statement, expect);
}

void _assertUsersInDB(const UserIdSet &excludeUserIdSet)
{
	string statement = "select * from ";
	statement += DBTablesUser::TABLE_NAME_USERS;
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		UserIdType userId = i + 1;
		if (excludeUserIdSet.find(userId) != excludeUserIdSet.end())
			continue;
		const UserInfo &userInfo = testUserInfo[i];
		expect += StringUtils::sprintf(
		  "%" FMT_USER_ID "|%s|%s|%" FMT_OPPRVLG "\n",
		  userId, userInfo.name.c_str(),
		  Utils::sha256(userInfo.password).c_str(),
		  userInfo.flags);
	}
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getUser().getDBAgent(), statement, expect);
}

void _assertAccessInfoInDB(const AccessInfoIdSet &excludeAccessInfoIdSet)
{
	string statement = "select * from ";
	statement += DBTablesUser::TABLE_NAME_ACCESS_LIST;
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		AccessInfoIdType id = i + 1;
		if (excludeAccessInfoIdSet.find(id) != excludeAccessInfoIdSet.end())
			continue;
		const AccessInfo &accessInfo = testAccessInfo[i];
		expect += StringUtils::sprintf(
		  "%" FMT_ACCESS_INFO_ID "|%" FMT_USER_ID "|%d|"
		  "%" FMT_HOST_GROUP_ID "\n",
		  id, accessInfo.userId, accessInfo.serverId,
		  accessInfo.hostgroupId.c_str());
	}
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getUser().getDBAgent(), statement, expect);
}

void _assertUserRoleInfoInDB(UserRoleInfo &userRoleInfo)
{
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBTablesUser::TABLE_NAME_USER_ROLES,
			     userRoleInfo.id);
	string expect = makeUserRoleInfoOutput(userRoleInfo);
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getUser().getDBAgent(), statement, expect);
}
#define assertUserRoleInfoInDB(I) cut_trace(_assertUserRoleInfoInDB(I))

void _assertUserRolesInDB(const UserRoleIdSet &excludeUserRoleIdSet)
{
	string statement = "select * from ";
	statement += DBTablesUser::TABLE_NAME_USER_ROLES;
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		UserRoleIdType userRoleId = i + 1;
		UserRoleIdSetIterator endIt = excludeUserRoleIdSet.end();
		if (excludeUserRoleIdSet.find(userRoleId) != endIt)
			continue;
		UserRoleInfo userRoleInfo = testUserRoleInfo[i];
		userRoleInfo.id = userRoleId;
		expect += makeUserRoleInfoOutput(userRoleInfo);
	}
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getUser().getDBAgent(), statement, expect);
}

void _assertIncidentTrackersInDB(const IncidentTrackerIdSet &excludeServerIdSet)
{
	string statement = "select * from incident_trackers ";
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestIncidentTrackerInfo; i++) {
		IncidentTrackerIdType incidentTrackerId = i + 1;
		IncidentTrackerInfo incidentTrackerInfo
		  = testIncidentTrackerInfo[i];
		incidentTrackerInfo.id = incidentTrackerId;
		ServerIdSetIterator it
		  = excludeServerIdSet.find(incidentTrackerId);
		if (it != excludeServerIdSet.end())
			continue;
		expect += makeIncidentTrackerInfoOutput(incidentTrackerInfo);
	}
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getConfig().getDBAgent(), statement, expect);
}

static bool makeTestDB(MYSQL *mysql, const string &dbName)
{
	string query = "CREATE DATABASE ";
	query += dbName;
	return mysql_query(mysql, query.c_str()) == 0;
}

static bool dropTestDB(MYSQL *mysql, const string &dbName)
{
	string query = "DROP DATABASE ";
	query += dbName;
	return mysql_query(mysql, query.c_str()) == 0;
}

void makeTestMySQLDBIfNeeded(const string &dbName, bool recreate)
{
	// make a connection
	const char *host = NULL; // localhost is used.
	const char *user = NULL; // current user name is used.
	const char *passwd = NULL; // passwd is not checked.
	const char *db = NULL;
	unsigned int port = 0; // default port is used.
	const char *unixSocket = NULL;
	unsigned long clientFlag = 0;
	MYSQL mysql;
	mysql_init(&mysql);
	MYSQL *succeeded = mysql_real_connect(&mysql, host, user, passwd,
	                                      db, port, unixSocket, clientFlag);
	if (!succeeded) {
		cut_fail("Failed to connect to MySQL: %s: %s\n",
		          db, mysql_error(&mysql));
	}

	// try to find the test database
	MYSQL_RES *result = mysql_list_dbs(&mysql, dbName.c_str());
	if (!result) {
		mysql_close(&mysql);
		cut_fail("Failed to list table: %s: %s\n",
		          db, mysql_error(&mysql));
	}

	MYSQL_ROW row;
	bool found = false;
	size_t lengthTestDBName = dbName.size();
	while ((row = mysql_fetch_row(result))) {
		if (dbName.compare(0, lengthTestDBName, row[0]) == 0) {
			found = true;
			break;
		}
	}
	mysql_free_result(result);

	// make DB if needed
	bool noError = false;
	if (!found) {
		if (!makeTestDB(&mysql, dbName))
			goto exit;
	} else if (recreate) {
		if (!dropTestDB(&mysql, dbName))
			goto exit;
		if (!makeTestDB(&mysql, dbName))
			goto exit;
	}
	noError = true;

exit:
	// close the connection
	string errmsg;
	if (!noError)
		errmsg = mysql_error(&mysql);
	mysql_close(&mysql);
	cppcut_assert_equal(true, noError,
	   cut_message("Failed to query: %s", errmsg.c_str()));
}

string
replaceForUtf8(const string &str, const string &target, const string &newChar)
{
	string ret;
	const gchar *orig = str.c_str();
	const gchar *curr = orig;
	gchar *next;
	while (true)
	{
		next = g_utf8_find_next_char(curr, NULL);
		if (curr == next)
			break;
		size_t len = next - curr;
		string currChar(curr, 0, len);
		if (currChar == target)
			ret += newChar;
		else
			ret += currChar;
		curr = next;
	}
	return ret;
}

string execSQL(DBAgent *dbAgent, const string &statement, bool showHeader)
{

	string output;
	const type_info& tid = typeid(*dbAgent);
	if (tid == typeid(DBAgentMySQL)) {
		DBAgentMySQL *dbMySQL = dynamic_cast<DBAgentMySQL *>(dbAgent);
		output = execMySQL(dbMySQL->getDBName(), statement, showHeader);
		output = replaceForUtf8(output, "\t", "|");
	} else if (tid == typeid(DBAgentSQLite3)) {
		DBAgentSQLite3 *dbSQLite3 =
		  dynamic_cast<DBAgentSQLite3 *>(dbAgent);
		cppcut_assert_equal(false, showHeader,
		  cut_message("Not implemented yet\n"));
		output = execSqlite3ForDBClient(
		           dbSQLite3->getDBPath(), statement);
	} else {
		cut_fail("Unknown type_info");
	}
	return output;
}

string joinStringVector(const StringVector &strVect, const string &pad,
                        bool isPaddingTail)
{
	string output;
	size_t numElem = strVect.size();
	size_t lastIndex = numElem - 1;
	for (size_t i = 0; i < numElem; i++) {
		output += strVect[i];
		if (i < lastIndex)
			output += pad;
		else if (i == lastIndex && isPaddingTail)
			output += pad;
	}
	return output;
}

string sortedJoin(const EventInfoList &events)
{
	set<string> strSet;
	for (auto &event: events)
		strSet.insert(makeEventOutput(event));
	string joined;
	for (auto &output : strSet)
		joined += output;
	return joined;
}

void crash(void)
{
	// Accessing the invalid address (NULL) finally causes SIGILL
	// when the compiler is clang, although we expect SIGSEGV.
	// So we send the signal directly when using clang.
	// Now hatohol is built with clang as a trial.
#ifdef __clang__
	kill(getpid(), SIGSEGV);
	while (true)
		sleep(1);
#else
	char *p = NULL;
	*p = 'a';
#endif
}

void prepareTestDataExcludeDefunctServers(void)
{
	gcut_add_datum("Don't exclude defunct servers",
		       "excludeDefunctServers", G_TYPE_BOOLEAN, FALSE,
		       NULL);
	gcut_add_datum("Exclude defunct servers",
		       "excludeDefunctServers", G_TYPE_BOOLEAN, TRUE,
		       NULL);
}

void fixupForFilteringDefunctServer(
  gconstpointer data, string &expected, HostResourceQueryOption &option,
  const string &tableName)
{
	const bool excludeDefunctServers =
	  gcut_data_get_boolean(data, "excludeDefunctServers");
	option.setExcludeDefunctServers(excludeDefunctServers);
	if (excludeDefunctServers)
		insertValidServerCond(expected, option, tableName);
}

void insertValidServerCond(
  string &condition, const HostResourceQueryOption &opt,
  const string &tableName)
{
	struct MyOption : public HostResourceQueryOption {
	public:
		MyOption(const HostResourceQueryOption &opt)
		: HostResourceQueryOption(opt)
		{
		}

		string makeServerCond(const string &tableName)
		{
			const ServerIdSet &serverIdSet =
			  getDataQueryContext().getValidServerIdSet();
			string cond = makeConditionServer(
			                serverIdSet, getServerIdColumnName());
			return cond;
		}

		void insertCond(string &svCond, const string &condition)
		{
			addCondition(svCond, condition);
		}
	};

	MyOption myOpt(opt);
	string svCond = myOpt.makeServerCond(tableName);
	myOpt.insertCond(svCond, condition);
	condition = svCond;
}

string makeDoubleFloatFormat(const ColumnDef &columnDef)
{
	return StringUtils::sprintf("%%.%zdlf", columnDef.decFracLength);
}

void initServerInfo(MonitoringServerInfo &serverInfo)
{
	serverInfo.id = 12345;
	serverInfo.type = MONITORING_SYSTEM_UNKNOWN;
	serverInfo.port = 0;
	serverInfo.pollingIntervalSec = 1;
	serverInfo.retryIntervalSec = 1;
}

void setTestValue(ArmInfo &armInfo)
{
	armInfo.running = true;
	armInfo.stat = ARM_WORK_STAT_FAILURE;
	armInfo.statUpdateTime = SmartTime(SmartTime::INIT_CURR_TIME);
	armInfo.failureComment = "How times have changed!";
	armInfo.lastSuccessTime = SmartTime();
	armInfo.lastFailureTime = SmartTime(SmartTime::INIT_CURR_TIME);
	armInfo.numUpdate  = 12345678;
	armInfo.numFailure = 543210;
}

static GMainContext *g_acquiredContext = NULL;
void _acquireDefaultContext(void)
{
	GMainContext *context = g_main_context_default();
	cppcut_assert_equal((gboolean)TRUE, g_main_context_acquire(context));
	g_acquiredContext = context;
}

void releaseDefaultContext(void)
{
	if (g_acquiredContext) {
		g_main_context_release(g_acquiredContext);
		g_acquiredContext = NULL;
	}
}

UserIdType searchMaxTestUserId(void)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	UserInfoList userInfoList;
	UserQueryOption option(USER_ID_SYSTEM);
	dbUser.getUserInfoList(userInfoList, option);
	cppcut_assert_equal(false, userInfoList.empty());

	UserIdType userId = 0;
	UserInfoListIterator userInfo = userInfoList.begin();
	for (; userInfo != userInfoList.end(); ++userInfo) {
		if (userInfo->id > userId)
			userId = userInfo->id;
	}
	return userId;
}

static UserIdType findUserCommon(const OperationPrivilegeType &type,
                                 const bool &shouldHas,
                                 const OperationPrivilegeFlag &excludeFlags = 0)
{
	UserIdType userId = INVALID_USER_ID;
	OperationPrivilegeFlag flag = OperationPrivilege::makeFlag(type);

	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	UserInfoList userInfoList;
	UserQueryOption option(USER_ID_SYSTEM);
	dbUser.getUserInfoList(userInfoList, option);
	cppcut_assert_equal(false, userInfoList.empty());

	UserInfoListIterator userInfo = userInfoList.begin();
	for (; userInfo != userInfoList.end(); ++userInfo) {
		if (userInfo->flags & excludeFlags)
			continue;
		bool having = userInfo->flags & flag;
		if ((shouldHas && having) || (!shouldHas && !having)) {
			userId = userInfo->id;
			break;
		}
	}
	cppcut_assert_not_equal(INVALID_USER_ID, userId);
	cppcut_assert_not_equal(USER_ID_SYSTEM, userId);
	return userId;
}


UserIdType findUserWith(const OperationPrivilegeType &type,
                        const OperationPrivilegeFlag &excludeFlags)
{
	return findUserCommon(type, true, excludeFlags);
}

UserIdType findUserWith(const OperationPrivilegeFlag &privFlag)
{
	UserIdType userId = INVALID_USER_ID;
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		const UserInfo &userInfo = testUserInfo[i];
		if (userInfo.flags == privFlag)
			userId = i + 1;
	}
	cppcut_assert_not_equal(INVALID_USER_ID, userId);
	cppcut_assert_not_equal(USER_ID_SYSTEM, userId);
	return userId;
}

UserIdType findUserWithout(const OperationPrivilegeType &type)
{
	return findUserCommon(type, false);
}

void initActionDef(ActionDef &actionDef)
{
       actionDef.id = 0;
       actionDef.type = ACTION_COMMAND;
       actionDef.timeout = 0;
       actionDef.ownerUserId = INVALID_USER_ID;;
}

string getSyslogTail(size_t numLines)
{
	// TODO: consider the environment that uses /var/log/messages.
	//       mlpl's testLogger does the similar things. We should
	//       create a commonly used method.
	const string cmd =
	  StringUtils::sprintf("tail -%zd /var/log/syslog", numLines);
	return executeCommand(cmd);
}

void _assertFileContent(const string &expect, const string &path)
{
	static const int TIMEOUT_SEC = 5;
	gchar *contents = NULL;
	gsize length;
	GError *error = NULL;
	SmartTime startTime(SmartTime::INIT_CURR_TIME);
	while (true) {
		g_file_get_contents(path.c_str(), &contents, &length, &error);
		gcut_assert_error(error);
		if (length > 0)
			break;

		// Some tests such as test_createWithEnv() sometimes fails
		// due to empty of file contents. Although the clear
		// reason is unknown, we try to read repeatedly in order to
		// avoid the failure.
		g_free(contents);
		usleep(10 * 1000); // 10ms
		SmartTime elapsed(SmartTime::INIT_CURR_TIME);
		elapsed -= startTime;
		cppcut_assert_equal(true, elapsed.getAsSec() < TIMEOUT_SEC);
	}
	Reaper<void> reaper(contents, g_free);
	cut_assert_equal_memory(expect.c_str(), expect.size(),
	                        contents, length);
}

void _assertGError(const GError *error)
{
	const char *errmsg = error ? error->message : "";
	cppcut_assert_null(error, cut_message("%s", errmsg));
}

ItemTablePtr convert(const ItemCategoryNameMap &itemCategoryNameMap)
{
	VariableItemTablePtr tablePtr;
	ItemCategoryNameMapConstIterator it = itemCategoryNameMap.begin();
	for (; it != itemCategoryNameMap.end(); ++it) {
		VariableItemGroupPtr grp;
		const ItemCategoryIdType &id = it->first;
		const string &name = it->second;
		grp->addNewItem(ITEM_ID_ZBX_APPLICATIONS_APPLICATIONID, id);
		grp->addNewItem(ITEM_ID_ZBX_APPLICATIONS_NAME, name);
		tablePtr->add(grp);
	}
	return static_cast<ItemTablePtr>(tablePtr);
}

VariableItemGroupPtr convert(const HistoryInfo &historyInfo)
{
	// TODO: Don't use IDs concerned with Zabbix.
	VariableItemGroupPtr grp;
	grp->addNewItem(ITEM_ID_ZBX_HISTORY_ITEMID,
			historyInfo.itemId);
	grp->addNewItem(ITEM_ID_ZBX_HISTORY_CLOCK,
			static_cast<uint64_t>(historyInfo.clock.tv_sec));
	grp->addNewItem(ITEM_ID_ZBX_HISTORY_NS,
			static_cast<uint64_t>(historyInfo.clock.tv_nsec));
	grp->addNewItem(ITEM_ID_ZBX_HISTORY_VALUE,
			historyInfo.value);
	return grp;
}

VariableItemGroupPtr convert(const TriggerInfo &triggerInfo)
{
	// TODO: Don't use IDs concerned with Zabbix.
	VariableItemGroupPtr grp;
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_TRIGGERID, triggerInfo.id);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_VALUE, triggerInfo.status);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_PRIORITY, triggerInfo.severity);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_LASTCHANGE,
	(int)(triggerInfo.lastChangeTime.tv_sec));
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_DESCRIPTION, triggerInfo.brief);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_HOSTID, triggerInfo.hostIdInServer);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_EXPANDED_DESCRIPTION, triggerInfo.extendedInfo);
	return grp;
}

// ---------------------------------------------------------------------------
// Watcher
// ---------------------------------------------------------------------------
Watcher::Watcher(void)
: expired(false),
  timerId(INVALID_EVENT_ID)
{
}

Watcher::~Watcher()
{
	if (timerId != INVALID_EVENT_ID)
		g_source_remove(timerId);
}

gboolean Watcher::_run(gpointer data)
{
	Watcher *obj = static_cast<Watcher *>(data);
	return obj->run();
}

gboolean Watcher::run(void)
{
	expired = true;
	timerId = INVALID_EVENT_ID;
	return G_SOURCE_REMOVE;
}

bool Watcher::start(const size_t &timeout, const size_t &interval)
{
	timerId = g_timeout_add(timeout, _run, this);
	while (!expired) {
		if (interval == 0) {
			g_main_context_iteration(NULL, TRUE);
		} else {
			while (g_main_context_iteration(NULL, FALSE))
				;
			usleep(interval);
		}
		if (watch())
			return true;
	}
	return false;
}

bool Watcher::watch(void)
{
	return false;
}

// ---------------------------------------------------------------------------
// LinesComparator
// ---------------------------------------------------------------------------
struct LinesComparator::PrivateContext
{
	multiset<string> set0;
	multiset<string> set1;

	string str0;
	string str1;

	void assertWithoutOrder(void)
	{
		multiset<string>::iterator line0Itr;
		multiset<string>::iterator line1Itr;
		while (!set0.empty()) {
			line0Itr = set0.begin();
			line1Itr = set1.find(*line0Itr);
			if (line1Itr == set1.end()) {
				cut_fail("Not found line: %s\n"
				         "<<set0>>\n%s\n"
				         "<<set1>>\n%s\n",
				         line0Itr->c_str(),
				         str0.c_str(), str1.c_str());
			}
			notifyIfVerboseMode(*line0Itr, *line1Itr);
			set0.erase(line0Itr);
			set1.erase(line1Itr);
		}
	}
};

LinesComparator::LinesComparator(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

LinesComparator::~LinesComparator()
{
	delete m_ctx;
}

void LinesComparator::add(
  const std::string &line0, const std::string &line1)
{
	m_ctx->str0 += line0;
	m_ctx->str0 += "\n";
	m_ctx->str1 += line1;
	m_ctx->str1 += "\n";

	m_ctx->set0.insert(line0);
	m_ctx->set1.insert(line1);
}

void LinesComparator::assert(const bool &strictOrder)
{
	if (strictOrder) {
		notifyIfVerboseMode(m_ctx->str0, m_ctx->str1);
		cppcut_assert_equal(m_ctx->str0, m_ctx->str1);
	} else {
		m_ctx->assertWithoutOrder();
	}
}

// ---------------------------------------------------------------------------
// GMainLoopAgent
// ---------------------------------------------------------------------------
GMainLoopAgent::GMainLoopAgent(void)
: m_timerTag(0),
  m_loop(NULL),
  m_timeoutCb(NULL),
  m_timeoutCbData(NULL)
{
}

GMainLoopAgent::~GMainLoopAgent()
{
	if (m_loop)
		g_main_loop_unref(m_loop);
	if (m_timerTag)
		g_source_remove(m_timerTag);
}

void GMainLoopAgent::run(GSourceFunc _timeoutCb, gpointer _timeoutCbData)
{
	cppcut_assert_equal(0u, m_timerTag);
	m_timeoutCb = _timeoutCb ? : NULL;
	m_timeoutCbData = _timeoutCbData ? : NULL;
	m_timerTag = g_timeout_add(TIMEOUT, timedOut, this);
	g_main_loop_run(get());
	if (m_timerTag) {
		g_source_remove(m_timerTag);
		m_timerTag = 0;
	}
}

void GMainLoopAgent::quit(void)
{
	g_main_loop_quit(get());
}

GMainLoop *GMainLoopAgent::get(void)
{
	m_lock.lock();
	if (!m_loop)
		m_loop = g_main_loop_new(NULL, TRUE);
	m_lock.unlock();
	return m_loop;
}

gboolean GMainLoopAgent::timedOut(gpointer data)
{
	GMainLoopAgent *obj = static_cast<GMainLoopAgent *>(data);
	if (obj->m_timeoutCb) {
		gboolean removeFlag = (*obj->m_timeoutCb)(obj->m_timeoutCbData);
		if (removeFlag == G_SOURCE_REMOVE)
			obj->m_timerTag = 0;
		return removeFlag;
	}
	obj->m_timerTag = 0;
	cut_fail("Timed out");
	return G_SOURCE_REMOVE;
}

// ---------------------------------------------------------------------------
// GLibMainLoop
// ---------------------------------------------------------------------------
GLibMainLoop::GLibMainLoop(GMainContext *context)
: m_context(NULL),
  m_loop(NULL)
{
	if (context)
		m_context = g_main_context_ref(context);
	else
		m_context = g_main_context_new();
	m_loop = g_main_loop_new(m_context, FALSE);
}

GLibMainLoop::~GLibMainLoop()
{
	if (m_loop) {
		g_main_loop_quit(m_loop);
		g_main_loop_unref(m_loop);
	}
	if (m_context)
		g_main_context_unref(m_context);
}

gpointer GLibMainLoop::mainThread(HatoholThreadArg *arg)
{
	g_main_loop_run(m_loop);
	return NULL;
}

GMainContext *GLibMainLoop::getContext(void) const
{
	return m_context;
}

GMainLoop *GLibMainLoop::getLoop(void) const
{
	return m_loop;
}

// ---------------------------------------------------------------------------
// CommandArgHelper
// ---------------------------------------------------------------------------
CommandArgHelper::CommandArgHelper(void)
{
	*this << "command-name";
}

bool CommandArgHelper::activate(void)
{
	gchar **argv = (gchar **)&args[0];
	gint argc = args.size();
	CommandLineOptions cmdLineOpts;
	const bool succeeded =
	  ConfigManager::parseCommandLine(&argc, (gchar ***)&argv,
	                                  &cmdLineOpts);
	bool loadConfigFile = true;
	ConfigManager::reset(&cmdLineOpts, !loadConfigFile);
	return succeeded;
}

void CommandArgHelper::operator <<(const char *word)
{
	args.push_back(word);
}

TestModeStone::TestModeStone(void)
{
	m_cmdArgHelper << "--test-mode";
	m_cmdArgHelper.activate();
}

// ---------------------------------------------------------------------------
// TentativeEnvVariable
// ---------------------------------------------------------------------------
TentativeEnvVariable::TentativeEnvVariable(const std::string &envVarName)
: m_envVarName(envVarName),
  m_hasOrigValue(false)
{
	const char *env = getenv(envVarName.c_str());
	if (!env)
		return;
	m_origString = env;
	m_hasOrigValue = true;;
}

TentativeEnvVariable::~TentativeEnvVariable()
{
	errno = 0;
	if (m_hasOrigValue)
		setenv(m_envVarName.c_str(), m_origString.c_str(), 1);
	else
		unsetenv(m_envVarName.c_str());
	cut_assert_errno();
}

string TentativeEnvVariable::getEnvVarName(void) const
{
	return m_envVarName;
}

void TentativeEnvVariable::set(const std::string &newValue)
{
	errno = 0;
	setenv(m_envVarName.c_str(), newValue.c_str(), 1);
	cut_assert_errno();
}
