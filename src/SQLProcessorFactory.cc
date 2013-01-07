#include "SQLProcessorFactory.h"
#include "SQLProcessorZabbix.h"

GRWLock SQLProcessorFactory::m_lock;
map<string, SQLProcessorCreatorFunc> SQLProcessorFactory::m_factoryMap;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessorFactory::init(void)
{
	addFactory(SQLProcessorZabbix::getDBNameStatic(),
	           SQLProcessorZabbix::createInstance);
}

SQLProcessor *SQLProcessorFactory::create(string &DBName)
{
	map<string, SQLProcessorCreatorFunc>::iterator it;
	SQLProcessor *composer = NULL;

	g_rw_lock_reader_lock(&m_lock);
	it = m_factoryMap.find(DBName);
	if (it != m_factoryMap.end()) {
		SQLProcessorCreatorFunc creator = it->second;
		composer = (*creator)();
	}
	g_rw_lock_reader_unlock(&m_lock);

	return composer;
}

void SQLProcessorFactory::addFactory(string name, SQLProcessorCreatorFunc factory)
{
	g_rw_lock_writer_lock(&m_lock);
	m_factoryMap[name] = factory;
	g_rw_lock_writer_unlock(&m_lock);
}
