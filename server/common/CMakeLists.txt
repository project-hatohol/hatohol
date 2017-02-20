include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(${LIBSOUP_INCLUDE_DIRS})
include_directories(${JSON_GLIB_INCLUDE_DIRS})

include_directories("../..")
include_directories("../mlpl/src")

add_library(libhatohol-common SHARED
	AMQPConnectionInfo.cc
	AMQPMessageHandler.cc
	ArmStatus.cc
	DataStoreException.cc
	HatoholError.cc
	HatoholException.cc
	HatoholThreadBase.cc
	ItemData.cc
	ItemDataUtils.cc
	ItemGroup.cc
	ItemGroupType.cc
	ItemTablePtr.cc
	JSONBuilder.cc
	JSONParser.cc
	JSONParserPositionStack.cc
	UsedCountable.cc
	UsedCountablePtr.cc
	ItemGroupPtr.cc
	ItemTable.cc
	ArmPluginInfo.cc
	MonitoringServerInfo.cc
	NamedPipe.cc
	Utils.cc
	AMQPConnection.cc
	AMQPConsumer.cc
	AMQPPublisher.cc
	HatoholArmPluginInterfaceHAPI2.cc
)
set_target_properties(libhatohol-common PROPERTIES PREFIX "")
