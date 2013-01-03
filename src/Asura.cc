#include "Asura.h"

#include "SQLProcessorZabbix.h"
#include "SQLProcessorFactory.h"
#include "FaceMySQLWorker.h"

void asuraInit(void)
{
	FaceMySQLWorker::init();
	SQLProcessorZabbix::init();
	SQLProcessorFactory::init();
}
