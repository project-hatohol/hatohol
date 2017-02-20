include_directories(${CUTTER_INCLUDE_DIRS})
include_directories(${CPP_CUTTER_INCLUDE_DIRS})

include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(${JSON_GLIB_INCLUDE_DIRS})
include_directories(${LIBSOUP_INCLUDE_DIRS})
include_directories("/usr/include/mysql")

include_directories(${PROJECT_BINARY_DIR})
include_directories("../..")
include_directories("../mlpl/src")
include_directories("../common")
include_directories("../src")

add_library(libtest STATIC
	DataSamples.cc
	DBTest.cc
	FaceRestTestUtils.cc
	Helpers.cc
	DBAgentTest.cc
	DBTablesTest.cc
	HttpServerStub.cc
	RedmineAPIEmulator.cc
	Synchronizer.cc
)
set_target_properties(libtest PROPERTIES PREFIX "")

add_library(libresident-test STATIC
	residentTest.cc
)
set_target_properties(libresident-test PROPERTIES PREFIX "")

add_library(libtest-hatohol STATIC
	testActionExecArgMaker.cc
	testActionManager.cc
	testActorCollector.cc
	testArmPluginInfo.cc
	testThreadLocalDBCache.cc
	testChildProcessManager.cc
	testConfigManager.cc
	testDataQueryContext.cc
	testDataQueryOption.cc
	testDataStoreManager.cc
	testDataStoreFactory.cc
	testHatoholError.cc
	testHatoholException.cc
	testHatoholThreadBase.cc
	testHatoholDBUtils.cc
	testHostInfoCache.cc
	TestHostResourceQueryOption.cc
	testHostResourceQueryOption.cc
	testHostResourceQueryOptionSubClasses.cc
	testDBAgent.cc
	testDBAgentSQLite3.cc
	testDBAgentMySQL.cc
	testDB.cc
	testDBTables.cc
	testDBClientUtils.cc
	testDBTablesConfig.cc
	testDBTablesUser.cc
	testDBTablesMonitoring.cc
	testDBTablesAction.cc
	testDBTablesHost.cc
	testDBTablesLastInfo.cc
	testDBClientJoinBuilder.cc
	testDBTermCodec.cc
	testDBTermCStringProvider.cc
	testOperationPrivilege.cc
	testSQLUtils.cc
	testFaceRest.cc
	testFaceRestCustomIncidentStatus.cc
	testFaceRestSeverityRank.cc
	testFaceRestSummary.cc
	testFaceRestSystem.cc
	testFaceRestAction.cc
	testFaceRestMonitoring.cc
	testFaceRestServer.cc
	testFaceRestUser.cc
	testFaceRestNoInit.cc
	testFaceRestIncident.cc
	testFaceRestIncidentTracker.cc
	testSessionManager.cc
	testIncidentSenderRedmine.cc
	testIncidentSenderHatohol.cc
	testIncidentSenderManager.cc
	testItemData.cc
	testItemGroup.cc
	testItemGroupStream.cc
	testItemDataPtr.cc
	testItemGroupType.cc
	testItemTable.cc
	testItemTablePtr.cc
	testItemDataUtils.cc
	testJSONParser.cc
	testJSONBuilder.cc
	testUtils.cc
	testJSONParserPositionStack.cc
	testNamedPipe.cc
	testSelfMonitor.cc
)
set_target_properties(libtest-hatohol PROPERTIES PREFIX "")

add_executable(ActionTp ActionTp.cc)
target_link_libraries(ActionTp
	${CUTTER_LIBRARIES}
	${CPP_CUTTER_LIBRARIES}
	${JSON_GLIB_LIBRARIES}
	${LIBSOUP_LIBRARIES}
	${SQLITE3_LIBRARIES}
	${GLIB2_LIBRARIES}
	${UUID_LIBRARIES}
	rabbitmq
	mysqlclient
	libtest
	libmlpl
	libhatohol
	libhatohol-common
)
