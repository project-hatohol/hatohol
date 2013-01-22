#include "Asura.h"
#include "glib.h"

#include "SQLProcessorZabbix.h"
#include "SQLProcessorFactory.h"
#include "FaceMySQLWorker.h"

static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
static bool initDone = false; 

void asuraInit(void)
{
	g_static_mutex_lock(&mutex);
	if (initDone) {
		g_static_mutex_unlock(&mutex);
		return;
	}

	FaceMySQLWorker::init();
	SQLProcessor::init();
	SQLProcessorZabbix::init();
	SQLProcessorFactory::init();

	initDone = true;
	g_static_mutex_unlock(&mutex);
}
