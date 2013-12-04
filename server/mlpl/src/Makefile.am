lib_LTLIBRARIES = libmlpl.la
libmlpl_la_SOURCES = \
	MutexLock.cc ReadWriteLock.cc \
	SmartBuffer.cc Logger.cc StringUtils.cc SmartQueue.cc \
	ParsableString.cc SmartTime.cc
 
AM_CXXFLAGS = -Wall -g3 -O2
AM_LDFLAGS = -lpthread -lrt

includedir = $(prefix)/include/mlpl
include_HEADERS = \
	AtomicValue.h \
	MutexLock.h ReadWriteLock.h \
	SmartBuffer.h Logger.h StringUtils.h SmartQueue.h ParsableString.h \
	SmartTime.h Reaper.h
