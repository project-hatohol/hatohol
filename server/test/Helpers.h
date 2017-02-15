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

#pragma once
#include <cppcutter.h>
#include <StringUtils.h>
#include <Mutex.h>
#include <SmartTime.h>
#include "ItemTable.h"
#include "DBAgent.h"
#include "HatoholError.h"
#include "HatoholThreadBase.h"
#include "OperationPrivilege.h"
#include "DBTablesConfig.h"
#include "DBTablesMonitoring.h"
#include "DBTablesAction.h"
#include "ArmStatus.h"

#define DBCONTENT_MAGIC_CURR_DATETIME "#CURR_DATETIME#"
#define DBCONTENT_MAGIC_CURR_TIME     "#CURR_TIME#"
#define DBCONTENT_MAGIC_NULL          "#NULL#"
#define DBCONTENT_MAGIC_ANY           "#ANY#"

typedef std::pair<int,int>      IntIntPair;
typedef std::vector<IntIntPair> IntIntPairVector;

void _assertStringVector(const mlpl::StringVector &expected,
                         const mlpl::StringVector &actual);
#define assertStringVector(E,A) cut_trace(_assertStringVector(E,A))

void _assertStringVectorVA(const mlpl::StringVector *actual, ...);
#define assertStringVectorVA(A,...) \
cut_trace(_assertStringVectorVA(A,##__VA_ARGS__))

template<typename T>
static std::shared_ptr<ItemTable> addItems(T* srcTable, int numTable,
                            void (*addFunc)(ItemGroup *, T *),
                            void (*tableCreatedCb)(std::shared_ptr<ItemTable>) = NULL)
{
	auto table = std::make_shared<ItemTable>();
	if (tableCreatedCb)
		(*tableCreatedCb)(table);
	for (int i = 0; i < numTable; i++) {
		ItemGroup* grp = new ItemGroup();
		(*addFunc)(grp, &srcTable[i]);
		table->add(grp, false);
	}
	return table;
}

template<typename T>
static void _assertItemData(const ItemGroup *itemGroup, const T &expected, int &idx)
{
	// TODO: replace with ItemGroupStream
	const ItemData *itemZ = itemGroup->getItemAt(idx);
	cut_assert_not_null(itemZ);
	idx++;
	T val = *itemZ;
	cut_trace(cppcut_assert_equal(expected, val));
}
#define assertItemData(T, IGRP, E, IDX) \
cut_trace(_assertItemData<T>(IGRP, E, IDX))

extern void _assertExist(const std::string &target, const std::string &words);
#define assertExist(T,W) cut_trace(_assertExist(T,W))

extern void _assertItemTable(
  const std::shared_ptr<const ItemTable> &expect, const std::shared_ptr<const ItemTable> &actual);
#define assertItemTable(E,A) cut_trace(_assertItemTable(E,A))

template<typename CT, typename KT>
void _assertHas(const CT &container, const KT &key)
{
	std::stringstream ss;
	const bool found = (container.find(key) != container.end());
	if (!found)
		ss << "Not found: " << key;
	cppcut_assert_equal(
	  true, found, cut_message("%s", ss.str().c_str()));
}
#define assertHas(C,K) cut_trace((_assertHas(C,K)))

template <typename T>
void assertEqualSet(const std::set<T> &expect, const std::set<T> &actual)
{
	auto errMsg = [&] {
		struct {
			std::string title;
			const std::set<T> &members;
		} args[] = {
			{"<expect>", expect}, {"<actual>", actual}
		};

		std::string s;
		for (const auto &a: args) {
			s += a.title + "\n";
			for (const auto &val: a.members)
				s += val + "\n";
		}
		return s;
	};

	cppcut_assert_equal(expect.size(), actual.size(),
	                    cut_message("%s\n", errMsg().c_str()));
	for (const auto &val : expect)
		assertHas(actual, val);
}

extern void _assertEqual(const std::set<int> &expect,
                         const std::set<int> &actual);
extern void _assertEqual(const std::set<std::string> &expect,
                         const std::set<std::string> &actual);
extern void _assertEqual(
  const MonitoringServerInfo &expect, const MonitoringServerInfo &actual);
extern void _assertEqual(const ArmInfo &expect, const ArmInfo &actual);
extern void _assertEqual(const ItemInfo &expect, const ItemInfo &actual);
extern void _assertEqual(const TriggerInfo &expect, const TriggerInfo &actual);
extern void _assertEqual(const ServerIdSet &expect, const ServerIdSet &actual);
extern void _assertEqual(const ServerHostGrpSetMap &expect,
                         const ServerHostGrpSetMap &actual);
extern void _assertEqual(const ServerHostSetMap &expect,
                         const ServerHostSetMap &actual);
#define assertEqual(E,A) cut_trace(_assertEqual(E,A))

extern void _assertEqualJSONString(
  const std::string &expect, const std::string &actual);
#define assertEqualJSONString(E,A) cut_trace(_assertEqualJSONString(E,A))

std::string executeCommand(const std::string &commandLine);
std::string getBaseDir(void);
std::string getFixturesDir(void);
bool isVerboseMode(void);

std::string getDBPathForDBClientHatohol(void);

std::string execMySQL(const std::string &dbName, const std::string &statement,
                      bool showHeader = false);

std::string makeServerInfoOutput(const MonitoringServerInfo &serverInfo);
std::string makeArmPluginInfoOutput(const ArmPluginInfo &armPluginInfo);
std::string makeIncidentTrackerInfoOutput(const IncidentTrackerInfo &incidentTrackerInfo);
std::string makeUserRoleInfoOutput(const UserRoleInfo &userRoleInfo);
std::string makeTriggerOutput(const TriggerInfo &triggerInfo);
std::string makeEventOutput(const EventInfo &eventInfo);
std::string makeIncidentOutput(const IncidentInfo &incidentInfo);
std::string makeHistoryOutput(const HistoryInfo &historyInfo);
std::string makeItemOutput(const ItemInfo &itemInfo);
std::string makeHostsOutput(const ServerHostDef &svHostDef, const size_t &id);
std::string makeHostgroupsOutput(const Hostgroup &hostgrp, const size_t &id);
std::string makeMapHostsHostgroupsOutput(const HostgroupMember &hostgrpMember, const size_t &id);
std::string makeSeverityRankInfoOutput(const SeverityRankInfo &severityRankInfo);
std::string makeCustomIncidentStatusOutput(const CustomIncidentStatus &customIncidentStatus);
std::string makeIncidentHistoryOutput(const IncidentHistory &incidentHistory);

void _assertDatetime(int expectedClock, int actualClock);
#define assertDatetime(E,A) cut_trace(_assertDatetime(E,A))

void _assertCurrDatetime(const std::string &datetime);
void _assertCurrDatetime(int datetime);
#define assertCurrDatetime(D) cut_trace(_assertCurrDatetime(D))

void _assertDBContent(DBAgent *dbAgent, const std::string &statement,
                      const std::string &expect);
#define assertDBContent(DB_AGENT, FMT, EXPECT) \
cut_trace(_assertDBContent(DB_AGENT, FMT, EXPECT))

void _assertCreateTable(DBAgent *dbAgent, const std::string &tableName);
#define assertCreateTable(DBAGENT,TBL_NAME) \
cut_trace(_assertCreateTable(DBAGENT,TBL_NAME))

void _assertExistIndex(
  DBAgent &dbAgent, const std::string &tableName, const std::string &indexName,
  const size_t &numColumns = 1);
#define assertExistIndex(DBAGENT, TBL_NAME, IDX_NAME, ...) \
cut_trace(_assertExistIndex(DBAGENT, TBL_NAME, IDX_NAME, ##__VA_ARGS__))

void _assertDBTablesVersion(DBAgent &dbAgent, const DBTablesId &tablesId,
                            const int &dbVersion);
#define assertDBTablesVersion(DBAGENT, TABLES_ID, DB_VERSION) \
cut_trace(_assertDBTablesVersion(DBAGENT, TABLES_ID, DB_VERSION))

void _assertTimeIsNow(const mlpl::SmartTime &smtime, double allowedError = 1);
#define assertTimeIsNow(ST, ...) cut_trace(_assertTimeIsNow(ST, ##__VA_ARGS__))

void _assertHatoholError(const HatoholErrorCode &code,
                         const HatoholError &err);
#define assertHatoholError(C,E) cut_trace(_assertHatoholError(C,E))

template<typename T> void _assertAddToDB(T *arg, void (*func)(T *))
{
	bool gotException = false;
	try {
		(*func)(arg);
	} catch (const HatoholException &e) {
		gotException = true;
		cut_fail("%s", e.getFancyMessage().c_str());
	} catch (...) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

void _assertServersInDB(const ServerIdSet &excludeServerIdSet = EMPTY_SERVER_ID_SET);
#define assertServersInDB(E) cut_trace(_assertServersInDB(E))

void _assertArmPluginsInDB(const std::set<int> &excludeIdSet);
#define assertArmPluginsInDB(E) cut_trace(_assertArmPluginsInDB(E))

void _assertIncidentTrackersInDB(const IncidentTrackerIdSet &excludeIncidentTrackerIdSet);
#define assertIncidentTrackersInDB(E) cut_trace(_assertIncidentTrackersInDB(E))

void _assertUsersInDB(const UserIdSet &excludeUserIdSet = EMPTY_USER_ID_SET);
#define assertUsersInDB(E) cut_trace(_assertUsersInDB(E))

void _assertAccessInfoInDB(const AccessInfoIdSet &excludeAccessInfoIdSet = EMPTY_ACCESS_INFO_ID_SET);
#define assertAccessInfoInDB(E) cut_trace(_assertAccessInfoInDB(E))

void _assertUserRoleInfoInDB(UserRoleInfo &userRoleInfo);
#define assertUserRoleInfoInDB(I) cut_trace(_assertUserRoleInfoInDB(I))
void _assertUserRolesInDB(const UserRoleIdSet &excludeUserRoleIdSet = EMPTY_USER_ROLE_ID_SET);
#define assertUserRolesInDB(E) cut_trace(_assertUserRolesInDB(E))

void makeTestMySQLDBIfNeeded(const std::string &dbName, bool recreate = false);
std::string execSQL(DBAgent *agent, const std::string &statement,
                    bool showHeader = false);
std::string joinStringVector(const mlpl::StringVector &strVect,
                             const std::string &pad = "",
                             bool isPaddingTail = true);

std::string sortedJoin(const EventInfoList &events);

void crash(void);
void prepareTestDataExcludeDefunctServers(void);
void fixupForFilteringDefunctServer(
  gconstpointer data, std::string &expected, HostResourceQueryOption &option,
  const std::string &tableName = "");
void insertValidServerCond(
  std::string &condition, const HostResourceQueryOption &opt,
  const std::string &tableName = "");
void initServerInfo(MonitoringServerInfo &serverInfo);
void setTestValue(ArmInfo &armInfo);

/**
 * Make a format string for a double float value that can be used for
 * printf() family functions. The number of decimals is columnDef.decFracLength.
 *
 * @param columnDef A reference of a columnDef instance.
 * @return A format string.
 *
 */
std::string makeDoubleFloatFormat(const ColumnDef &columnDef);

void _acquireDefaultContext(void);
#define acquireDefaultContext() cut_trace(_acquireDefaultContext())
void releaseDefaultContext(void);

UserIdType searchMaxTestUserId(void);
UserIdType findUserWith(const OperationPrivilegeType &type,
                        const OperationPrivilegeFlag &excludeFlags = 0);
UserIdType findUserWith(const OperationPrivilegeFlag &privFlag);
UserIdType findUserWithout(const OperationPrivilegeType &type);
void initActionDef(ActionDef &actionDef);
std::string getSyslogTail(size_t numLines);

void _assertFileContent(const std::string &expect, const std::string &path);
#define assertFileContent(E,P) cut_trace(_assertFileContent(E,P))

void _assertGError(const GError *error);
#define assertGError(E) cut_trace(_assertGError(E))

template<typename T0, typename T1>
std::string _makeElementsComparisonString(const T0 &exp, const T1 &act)
{
	std::stringstream ss;
	ss << "<<expect>>\n";
	typename T0::const_iterator it0;
	for (it0 = exp.begin(); it0 != exp.end(); ++it0) {
		ss << *it0;
		ss << ",";
	}
	typename T1::const_iterator it1;
	ss << "\n<<actual>>\n";
	for (it1 = act.begin(); it1 != act.end(); ++it1) {
		ss << *it1;
		ss << ",";
	}
	return ss.str();
}
#define makeElementsComparisonString(E,A) \
  _makeElementsComparisonString<__typeof__(E), __typeof__(A)>(E,A)

template<typename T0, typename T1>
void _assertEqualSize(const T0 &exp, const T1 &act)
{
	std::string errMsg;
	if (exp.size() != act.size())
		errMsg = makeElementsComparisonString(exp, act);
	cppcut_assert_equal(exp.size(), act.size(),
	                    cut_message("%s", errMsg.c_str()));
}
#define assertEqualSize(E, A) \
  cut_trace((_assertEqualSize<__typeof__(E), __typeof__(A)>(E,A)))

std::shared_ptr<const ItemTable> convert(const ItemCategoryNameMap &itemCategoryNameMap);

VariableItemGroupPtr convert(const HistoryInfo &historyInfo);
VariableItemGroupPtr convert(const TriggerInfo &triggerInfo);

class Watcher {
	bool expired;
	guint timerId;

	static gboolean _run(gpointer data);

protected:
	virtual gboolean run(void);
	virtual bool watch(void);

public:
	Watcher(void);
	virtual ~Watcher();

	/**
	 * Start a watch loop.
	 * NOTE: this function must be called on the default GLIB context.
	 *
	 * @ @param timeout
	 * A timeout value in milli-second.
	 *
	 * @param interval
	 * A watch interval in micro-second. If this parameter is zero,
	 * the watch is peformed every return of
	 * g_main_context_iteration(NULL, FALSE).
	 *
	 * @return
	 * If watch() returns true, this function immediately returns true.
	 * Othewise false is returned after the timeout.
	 */
	bool start(const size_t &timeout, const size_t &interval = 0);
};

class LinesComparator {
public:
	LinesComparator(void);
	virtual ~LinesComparator();
	void add(const std::string &line0, const std::string &line1);
	void assert(const bool &strictOrder = true);
private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

class GMainLoopAgent {
public:
	GMainLoopAgent(void);
	virtual ~GMainLoopAgent(void);

	/**
	 * Start the main loop run with a timeout.
	 *
	 * @param timeoutCb
	 * A callback function that is called at the timeout, If this
	 * parameters is NULL, default callback function that just call
	 * cut_fail() will be used.
	 *
	 * @param timeoutCbData
	 * Arbitary pointer to be passed to timeoutCb.
	 */
	virtual void run(GSourceFunc timeoutCb = NULL,
	                 gpointer timeoutCbData = NULL);
	virtual void quit(void);
	GMainLoop   *get(void);

protected:
	static gboolean timedOut(gpointer data);
private:
	static const size_t TIMEOUT = 5000;
	guint           m_timerTag;
	GMainLoop      *m_loop;
	mlpl::Mutex     m_lock;
	GSourceFunc     m_timeoutCb;
	gpointer        m_timeoutCbData;
};

class GLibMainLoop : public HatoholThreadBase {
public:
	GLibMainLoop(GMainContext *context = NULL);
	virtual ~GLibMainLoop();
	virtual gpointer mainThread(HatoholThreadArg *arg) override;
	GMainContext *getContext(void) const;
	GMainLoop    *getLoop(void) const;
private:
	GMainContext *m_context;
	GMainLoop    *m_loop;
};

struct CommandArgHelper
{
	std::vector<const char *> args;

	CommandArgHelper(void);
	bool activate(void);
	void operator <<(const char *word);
};

/**
 * Set test mode while this class's instance is alive.
 * The state is reset when the instance is deleted.
 */
class TestModeStone
{
public:
	TestModeStone(void);
private:
	CommandArgHelper m_cmdArgHelper;
};

class TentativeEnvVariable
{
public:
	TentativeEnvVariable(const std::string &envVarName);
	virtual ~TentativeEnvVariable();
	void set(const std::string &newValue);
	std::string getEnvVarName(void) const;
private:
	std::string m_envVarName;
	std::string m_origString;
	bool        m_hasOrigValue;
};

