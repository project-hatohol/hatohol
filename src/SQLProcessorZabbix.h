#ifndef SQLProcessorZabbix_h
#define SQLProcessorZabbix_h

#include "SQLProcessor.h"

class SQLProcessorZabbix : public SQLProcessor
{
public:
	static SQLProcessor *createInstance(void);
	SQLProcessorZabbix(void);
	~SQLProcessorZabbix();

protected:
};

#endif // SQLProcessorZabbix_h

