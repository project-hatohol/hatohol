#ifndef SQLProcessorZabbix_h
#define SQLProcessorZabbix_h

#include "ItemData.h"
#include "SQLProcessor.h"
#include "VirtualDataStoreZabbix.h"

class SQLProcessorZabbix : public SQLProcessor
{
public:
	// static methods
	static void init(void);
	static SQLProcessor *createInstance(void);
	static const char *getDBNameStatic(void);

	// constructor and desctructor
	SQLProcessorZabbix(void);
	~SQLProcessorZabbix();

	// virtual methods
	const char *getDBName(void);

private:
	static TableNameStaticInfoMap m_tableNameStaticInfoMap;

	static SQLTableStaticInfo *
	defineTable(int tableId, const char *tableName,
	            SQLTableMakeFunc tableMakeFunc);
	static void defineColumn(SQLTableStaticInfo *staticInfo,
	                         ItemId itemId,
	                         int tableId, const char *columnName,
	                         SQLColumnType, size_t columnLength);
	VirtualDataStoreZabbix *m_VDSZabbix;

	template<ItemGroupId GROUP_ID>
	bool tableMakeFuncTemplate(SQLSelectInfo &selectInfo,
	                           SQLTableInfo &tableInfo);
};

#endif // SQLProcessorZabbix_h
