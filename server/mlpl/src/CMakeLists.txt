add_library(libmlpl SHARED
	Mutex.cc
	ReadWriteLock.cc
	SimpleSemaphore.cc
	EventSemaphore.cc
	SeparatorInjector.cc
	SmartBuffer.cc
	Logger.cc
	StringUtils.cc
	ParsableString.cc
	SmartTime.cc
)
set_target_properties(libmlpl PROPERTIES PREFIX "")
