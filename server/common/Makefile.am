lib_LTLIBRARIES = libhatohol-common.la

libhatohol_common_la_SOURCES = \
	ArmPluginInfo.cc ArmPluginInfo.h \
	ArmStatus.cc ArmStatus.h \
	DataStoreException.cc DataStoreException.h \
	EndianConverter.h \
	HatoholThreadBase.cc HatoholThreadBase.h \
	HatoholException.cc HatoholException.h \
	HatoholError.cc HatoholError.h \
	ItemData.cc ItemData.h \
	ItemDataPtr.h \
	ItemEnum.h \
	ItemDataUtils.cc ItemDataUtils.h \
	ItemGroup.cc ItemGroup.h \
	ItemGroupType.cc ItemGroupType.h \
	ItemGroupPtr.cc ItemGroupPtr.h \
	ItemTable.cc ItemTable.h \
	JSONBuilder.cc JSONBuilder.h \
	JSONParser.cc JSONParser.h \
	JSONParserPositionStack.cc \
	Monitoring.h \
	MonitoringServerInfo.cc MonitoringServerInfo.h \
	NamedPipe.cc NamedPipe.h \
	Params.h \
	EndianConverter.h \
	UsedCountable.cc UsedCountable.h \
	Utils.cc Utils.h \
	UsedCountablePtr.cc UsedCountablePtr.h \
	AMQPConnectionInfo.cc AMQPConnectionInfo.h \
	AMQPConnection.cc AMQPConnection.h \
	AMQPConsumer.cc AMQPConsumer.h \
	AMQPPublisher.cc AMQPPublisher.h \
	AMQPMessageHandler.cc AMQPMessageHandler.h \
	HatoholArmPluginInterfaceHAPI2.cc HatoholArmPluginInterfaceHAPI2.h

AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(GIO_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS) \
	$(UUID_CFLAGS) $(LIBRABBITMQ_CFLAGS) \
	$(COVERAGE_CFLAGS)

AM_LDFLAGS = -lrt \
	$(MLPL_LIBS) $(GLIB_LIBS) $(GTHREAD_LIBS) $(GIO_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS) \
	$(UUID_LIBS) $(LIBRABBITMQ_LIBS) \
	$(COVERAGE_LIBS)
