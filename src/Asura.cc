#include "Asura.h"

#include "SQLProcessorZabbix.h"
#include "SQLProcessorFactory.h"

void asuraInit(void)
{
	SQLProcessorZabbix::init();
	SQLProcessorFactory::init();
}
