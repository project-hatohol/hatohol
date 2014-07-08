lib_LTLIBRARIES = libmlpl.la
libmlpl_la_SOURCES = \
	MutexLock.cc ReadWriteLock.cc SimpleSemaphore.cc EventSemaphore.cc \
	SmartBuffer.cc Logger.cc StringUtils.cc \
	ParsableString.cc SmartTime.cc
 
AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(COVERAGE_CFLAGS)
AM_LDFLAGS = -lpthread -lrt $(COVERAGE_LIBS)

includedir = $(prefix)/include/mlpl
include_HEADERS = \
	AtomicValue.h \
	MutexLock.h ReadWriteLock.h SimpleSemaphore.h EventSemaphore.h \
	SmartBuffer.h Logger.h StringUtils.h SmartQueue.h ParsableString.h \
	SmartTime.h Reaper.h
