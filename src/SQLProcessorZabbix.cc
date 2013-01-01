#include <Logger.h>
using namespace mlpl;

#include "SQLProcessorZabbix.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLProcessor *SQLProcessorZabbix::createInstance(void)
{
	return new SQLProcessorZabbix();
}

SQLProcessorZabbix::SQLProcessorZabbix(void)
{
	MLPL_INFO("created: %s\n", __func__);
}

SQLProcessorZabbix::~SQLProcessorZabbix()
{
}
