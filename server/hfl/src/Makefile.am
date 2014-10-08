lib_LTLIBRARIES = libhfl.la
libhfl_la_SOURCES = \
	Mutex.cc ReadWriteLock.cc SimpleSemaphore.cc EventSemaphore.cc \
	SeparatorInjector.cc \
	SmartBuffer.cc Logger.cc StringUtils.cc \
	ParsableString.cc SmartTime.cc
 
AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(COVERAGE_CFLAGS)
AM_LDFLAGS = -lpthread -lrt $(COVERAGE_LIBS)

includedir = $(prefix)/include/hfl
include_HEADERS = \
	AtomicValue.h \
	Mutex.h ReadWriteLock.h SimpleSemaphore.h EventSemaphore.h \
	SeparatorInjector.h \
	SmartBuffer.h Logger.h StringUtils.h SmartQueue.h ParsableString.h \
	SmartTime.h Reaper.h
