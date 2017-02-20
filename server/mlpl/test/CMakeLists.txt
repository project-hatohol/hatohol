include_directories(${CUTTER_INCLUDE_DIRS})
include_directories(${CPP_CUTTER_INCLUDE_DIRS})

include_directories(${GLIB2_INCLUDE_DIRS})

include_directories("../src")

add_library(libtestmlpl STATIC
	testMutex.cc
	testReadWriteLock.cc
	testLogger.cc
	testStringUtils.cc
	testParsableString.cc
	testSeparatorInjector.cc
	testSmartBuffer.cc
	testReaper.cc
	testSmartTime.cc
	testSmartQueue.cc
	testAtomicValue.cc
	testSimpleSemaphore.cc
	testEventSemaphore.cc
)
set_target_properties(libtestmlpl PROPERTIES PREFIX "")

add_executable(loggerTestee loggerTestee.cc)
target_link_libraries(loggerTestee
	libtestmlpl
	libmlpl
	pthread
)
