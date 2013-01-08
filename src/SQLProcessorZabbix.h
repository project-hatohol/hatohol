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

protected:
	//
	// Table Processors
	//
	bool tableProcNodes
	     (SQLSelectResult &result, SQLSelectInfo &selectInfo);

private:
	static TableProcFuncMap            m_tableProcFuncMap;
	static TableIdColumnBaseDefListMap m_tableColumnBaseDefListMap;
	static TableIdNameMap              m_tableIdNameMap;

	static void defineTable(int tableId, const char *tableName);
	static void defineColumn(ItemId itemId,
	                         int tableId, const char *columnName,
	                         SQLColumnType, size_t columnLength);
	static const char *getTableName(int tableId);
	VirtualDataStoreZabbix *m_VDSZabbix;

	template<ItemGroupId GROUP_ID, int TABLE_ID> 
	bool tableProcTemplate(SQLSelectResult &result,
	                       SQLSelectInfo &selectInfo,
	                       const SQLColumnInfo &columnInfo);
};

#endif // SQLProcessorZabbix_h
