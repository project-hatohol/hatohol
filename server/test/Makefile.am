SUBDIRS = . fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = libTest.la testHatohol.la residentTest.la
noinst_PROGRAMS = ActionTp
endif

# default
AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(CUTTER_CFLAGS) $(CPPCUTTER_CFLAGS) \
	$(GLIB_CFLAGS) $(MLPL_CFLAGS) $(JSON_GLIB_CFLAGS) $(LIBSOUP_CFLAGS) \
	$(MYSQL_CFLAGS) \
	-I $(top_srcdir)/server/src \
	-I $(top_srcdir)/server/common \
	-I $(top_srcdir)/server/hap
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

LIBS = \
	$(CUTTER_LIBS) $(CPPCUTTER_LIBS) \
	$(GLIB_LIBS) $(GIO_LIBS) $(MLPL_LIBS) \
	$(top_builddir)/server/src/libhatohol.la \
	$(top_builddir)/server/common/libhatohol-common.la

# Test cases
testHatohol_la_SOURCES = \
	testActionExecArgMaker.cc testActionManager.cc \
	testActorCollector.cc \
	testArmPluginInfo.cc \
	testThreadLocalDBCache.cc \
	testChildProcessManager.cc \
	testConfigManager.cc \
	testDataQueryContext.cc testDataQueryOption.cc \
	testDataStoreManager.cc testDataStoreFactory.cc \
	testHatoholError.cc \
	testHatoholException.cc \
	testHatoholThreadBase.cc \
	testHatoholDBUtils.cc \
	testHostInfoCache.cc \
	TestHostResourceQueryOption.cc TestHostResourceQueryOption.h \
	testHostResourceQueryOption.cc \
	testHostResourceQueryOptionSubClasses.cc \
	testDBAgent.cc \
	testDBAgentSQLite3.cc testDBAgentMySQL.cc \
	testDB.cc \
	testDBTables.cc \
	testDBClientUtils.cc \
	testDBTablesConfig.cc testDBTablesUser.cc \
	testDBTablesMonitoring.cc testDBTablesMonitoring.h \
	testDBTablesAction.cc \
	testDBTablesHost.cc \
	testDBTablesLastInfo.cc \
	testDBClientJoinBuilder.cc \
	testDBTermCodec.cc \
	testDBTermCStringProvider.cc \
	testOperationPrivilege.cc \
	testSQLUtils.cc \
	testFaceRest.cc \
	testFaceRestCustomIncidentStatus.cc \
	testFaceRestSeverityRank.cc testFaceRestSummary.cc \
	testFaceRestSystem.cc testFaceRestAction.cc testFaceRestMonitoring.cc \
	testFaceRestServer.cc testFaceRestUser.cc testFaceRestNoInit.cc \
	testFaceRestIncident.cc \
	testFaceRestIncidentTracker.cc testSessionManager.cc \
	testIncidentSenderRedmine.cc \
	testIncidentSenderHatohol.cc \
	testIncidentSenderManager.cc \
	testItemData.cc testItemGroup.cc testItemGroupStream.cc \
	testItemDataPtr.cc testItemGroupType.cc testItemTable.cc \
	testItemTablePtr.cc \
	testItemDataUtils.cc \
	testJSONParser.cc testJSONBuilder.cc testUtils.cc \
	testJSONParserPositionStack.cc \
	testNamedPipe.cc \
	testSelfMonitor.cc \
	testArmUtils.cc testArmBase.cc \
	testArmRedmine.cc \
	testArmStatus.cc testStatisticsCounter.cc \
	testUsedCountable.cc \
	testUnifiedDataStore.cc testMain.cc \
	testAMQPConnectionInfo.cc \
	testAMQPConnection.cc \
	testGateJSONEventMessage.cc \
	testHatoholArmPluginGateHAPI2.cc

testHatohol_la_LDFLAGS = \
	$(AM_LDFLAGS) $(LIBS)
testHatohol_la_LIBADD = libTest.la

residentTest_la_SOURCES = residentTest.cc residentTest.h
residentTest_la_LDFLAGS = $(AM_LDFLAGS) $(LIBS)
residentTest_la_LIBADD  = libTest.la

libTest_la_SOURCES = \
	DataSamples.cc DataSamples.h \
	DBTest.cc DBTest.h \
	FaceRestTestUtils.cc FaceRestTestUtils.h \
	Helpers.cc DBAgentTest.cc DBTablesTest.cc \
	HttpServerStub.cc RedmineAPIEmulator.cc \
	Synchronizer.cc \
	Helpers.h DBAgentTest.h DBTablesTest.h \
	HttpServerStub.h RedmineAPIEmulator.h \
	Synchronizer.h \
	ExceptionTestUtils.h AssertJoin.h \
	MultiLangTest.h

libTest_la_LDFLAGS =

ActionTp_SOURCES = ActionTp.cc ActionTp.h
ActionTp_LDFLAGS = $(LIBS)
ActionTp_LDADD = libTest.la

echo-cutter:
	@echo $(CUTTER)
