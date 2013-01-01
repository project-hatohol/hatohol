#ifndef SQLProcessorFactory_h
#define SQLProcessorFactory_h

#include <string>
#include <map>
using namespace std;

#include <glib.h>
#include "SQLProcessor.h"

typedef SQLProcessor *(*SQLProcessorCreatorFunc)(void);

class SQLProcessorFactory
{
public:
	static void init(void);
	static SQLProcessor *create(string &DBName);
	static void addFactory(string name, SQLProcessorCreatorFunc factory);

protected:

private:
	static GRWLock m_lock;
	static map<string, SQLProcessorCreatorFunc> m_factoryMap;
};

#endif // SQLProcessorFactory_h
