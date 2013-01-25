#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <algorithm>
using namespace std;

#include <stdexcept>
#include <cstring>
#include "SQLProcessor.h"
#include "Utils.h"

const SQLProcessor::SelectSubParser SQLProcessor::m_selectSubParsers[] = {
	&SQLProcessor::parseSelectedColumns,
	&SQLProcessor::parseGroupBy,
	&SQLProcessor::parseFrom,
	&SQLProcessor::parseWhere,
	&SQLProcessor::parseOrderBy,
	&SQLProcessor::parseLimit,
};

map<string, SQLProcessor::SelectSubParser>
  SQLProcessor::m_selectSectionParserMap;

map<string, SQLProcessor::SelectSubParser>
  SQLProcessor::m_whereKeywordHandlerMap;

enum BetweenParsingStep {
	BETWEEN_NONE,
	BETWEEN_EXPECT_FIRST,
	BETWEEN_EXPECT_AND,
	BETWEEN_EXPECT_SECOND,
};

struct SQLProcessor::SelectParserContext {
	SQLProcessor       *sqlProcessor;

	string              currWord;
	string              currWordLower;
	SelectParseSection  section;
	size_t              indexInTheStatus;
	SQLSelectInfo      &selectInfo;

	// used in where section
	bool                quotOpen;
	BetweenParsingStep  betweenStep;
	PolytypeNumber      betweenFirstValue;

	// methods
	SelectParserContext(SQLProcessor *sqlProc,
	                    SelectParseSection _section,
	                    SQLSelectInfo &_selectInfo)
	: sqlProcessor(sqlProc),
	  section(_section),
	  indexInTheStatus(0),
	  selectInfo(_selectInfo),
	  quotOpen(false),
	  betweenStep(BETWEEN_NONE)
	{
	}
};

struct WhereColumnArg {
	SQLSelectInfo &selectInfo;
	ItemId        itemId;

	// constructor
	WhereColumnArg(SQLSelectInfo &_selectInfo)
	: selectInfo(_selectInfo),
	  itemId(-1)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods (SQLColumnDefinitino)
// ---------------------------------------------------------------------------
SQLColumnDefinition::SQLColumnDefinition(void)
: columnBaseDef(NULL),
  tableInfo(NULL)
{
}

// ---------------------------------------------------------------------------
// Public methods (SQLTableInfo)
// ---------------------------------------------------------------------------
SQLTableInfo::SQLTableInfo(void)
: staticInfo(NULL)
{
}

// ---------------------------------------------------------------------------
// Public methods (SQLColumnInfo)
// ---------------------------------------------------------------------------
SQLColumnInfo::SQLColumnInfo(void)
: tableInfo(NULL),
  columnBaseDef(NULL),
  columnType(COLUMN_TYPE_UNKNOWN)
{
}

void SQLColumnInfo::associate(SQLTableInfo *_tableInfo)
{
	tableInfo = _tableInfo;
	_tableInfo->columnList.push_back(this);
}

void SQLColumnInfo::setColumnType(void)
{
	if (name == "*")
		columnType = SQLColumnInfo::COLUMN_TYPE_ALL;
	else if (baseName == "*")
		columnType = SQLColumnInfo::COLUMN_TYPE_ALL;
	else
		columnType = SQLColumnInfo::COLUMN_TYPE_NORMAL;
}

// ---------------------------------------------------------------------------
// Public methods (SQLSelectInfo)
// ---------------------------------------------------------------------------
SQLSelectInfo::SQLSelectInfo(ParsableString &_query)
: query(_query),
  useIndex(false),
  evalTargetItemGroup(NULL)
{
	rootWhereElem = new SQLWhereElement();
	currWhereElem = rootWhereElem;
}

SQLSelectInfo::~SQLSelectInfo()
{
	SQLColumnInfoVectorIterator columnIt = columns.begin();
	for (; columnIt != columns.end(); ++columnIt)
		delete *columnIt;

	SQLTableInfoVectorIterator tableIt = tables.begin();
	for (; tableIt != tables.end(); ++tableIt)
		delete *tableIt;

	if (rootWhereElem)
		delete rootWhereElem;;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessor::init(void)
{
	m_selectSectionParserMap["from"]  = &SQLProcessor::parseSectionFrom;
	m_selectSectionParserMap["where"] = &SQLProcessor::parseSectionWhere;
	m_selectSectionParserMap["order"] = &SQLProcessor::parseSectionOrder;
	m_selectSectionParserMap["group"] = &SQLProcessor::parseSectionGroup;
	m_selectSectionParserMap["limit"] = &SQLProcessor::parseSectionLimit;

	m_whereKeywordHandlerMap["and"] = &SQLProcessor::whereHandlerAnd;
	m_whereKeywordHandlerMap["between"] = &SQLProcessor::whereHandlerBetween;

	// check the size of m_selectSubParsers
	size_t size = sizeof(SQLProcessor::m_selectSubParsers) / 
	                sizeof(SelectSubParser);
	if (size != NUM_SELECT_PARSING_SECTION) {
		string msg;
		TRMSG(msg, "sizeof(m_selectSubParsers) is invalid: "
		           "(expcect/actual: %d/%d).",
		      NUM_SELECT_PARSING_SECTION, size);
		throw logic_error(msg);
	}
}

bool SQLProcessor::select(SQLSelectInfo &selectInfo)
{
	// disassemble the query statement
	if (!parseSelectStatement(selectInfo))
		return false;
	if (!checkParsedResult(selectInfo))
		return false;

	// associate each column with the table
	if (!associateColumnWithTable(selectInfo))
		return false;

	// associate each table with static table information
	if (!associateTableWithStaticInfo(selectInfo))
		return false;

	// make ItemTable objects for all specified tables
	if (!setColumnTypeAndBaseDefInColumnInfo(selectInfo))
		return false;
	if (!makeColumnDefs(selectInfo))
		return false;
	if (!makeItemTables(selectInfo))
		return false;

	// join tables
	if (!doJoin(selectInfo))
		return false;

	// add associated data to whereColumn to call evaluate()
	if (!fixupWhereColumn(selectInfo))
		return false;

	// pickup matching rows
	if (!selectMatchingRows(selectInfo))
		return false;

	// packed the requested columns
	if (!selectInfo.joinedTable->foreach<SQLSelectInfo&>
	                                    (packRequiredColumns, selectInfo))
		return false;

	// convert data to string
	if (!selectInfo.selectedTable->foreach<SQLSelectInfo&>(makeTextRows,
	                                                       selectInfo))
		return false;

	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SQLProcessor::SQLProcessor(TableNameStaticInfoMap &tableNameStaticInfoMap)
: m_separatorSpaceComma(" ,"),
  m_separatorCountSpaceComma(", "),
  m_separatorCBForWhere(" ='"),
  m_tableNameStaticInfoMap(tableNameStaticInfoMap)
{
	// m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_COLUMN]
	// is set later in parseSelectStatement().
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_GROUP_BY] = 
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_FROM] =
	  &m_separatorCountSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_WHERE] = 
	  &m_separatorCBForWhere;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_ORDER_BY] = 
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_LIMIT] = 
	  &m_separatorSpaceComma;
}

SQLProcessor::~SQLProcessor()
{
}

bool
SQLProcessor::checkSelectedAllColumns(const SQLSelectInfo &selectInfo,
                                      const SQLColumnInfo &columnInfo) const
{
	if (columnInfo.name == "*")
		return true;
	if (!columnInfo.tableInfo)
		return false;
	if (columnInfo.baseName == "*")
		return true;
	return false;
}

bool SQLProcessor::parseSelectStatement(SQLSelectInfo &selectInfo)
{
	MLPL_DBG("<%s> %s\n", __func__, selectInfo.query.getString());
	map<string, SelectSubParser>::iterator it;
	SelectSubParser subParser = NULL;
	SelectParserContext ctx(this, SELECT_PARSING_SECTION_COLUMN,
	                        selectInfo);

	// callback function for column
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_COLUMN]
	  = selectInfo.columnParser.getSeparatorChecker();

	// callback function for where section
	m_separatorCBForWhere.setCallbackTempl<SelectParserContext>
	                                      ('=', whereCbEq, &ctx);
	m_separatorCBForWhere.setCallbackTempl<SelectParserContext>
	                                      ('\'', whereCbQuot, &ctx);

	while (!selectInfo.query.finished()) {
		ctx.currWord = readNextWord(ctx);
		if (ctx.currWord.empty())
			continue;

		// check if this is a keyword.
		ctx.currWordLower = ctx.currWord;
		transform(ctx.currWordLower.begin(), ctx.currWordLower.end(),
		          ctx.currWordLower.begin(), ::tolower);
		
		it = m_selectSectionParserMap.find(ctx.currWordLower);
		if (it != m_selectSectionParserMap.end()) {
			// When the function returns 'true', it means
			// the current word is section keyword and
			subParser = it->second;
			if ((this->*subParser)(ctx)) {
				ctx.indexInTheStatus = 0;
				continue;
			}
		}

		// parse each component
		if (ctx.section >= NUM_SELECT_PARSING_SECTION) {
			MLPL_BUG("section(%d) >= NUM_SELECT_PARSING_SECTION\n",
			         ctx.section);
			return false;
		}
		subParser = m_selectSubParsers[ctx.section];
		if (!(this->*subParser)(ctx))
			return false;

		ctx.indexInTheStatus++;
	}

	return true;
}

bool SQLProcessor::checkParsedResult(const SQLSelectInfo &selectInfo) const
{
	if (selectInfo.columns.empty()) {
		MLPL_DBG("Not found: columns.\n");
		return false;
	}

	if (selectInfo.tables.empty()) {
		MLPL_DBG("Not found: tables.\n");
		return false;
	}

	return true;
}

bool SQLProcessor::associateColumnWithTable(SQLSelectInfo &selectInfo)
{
	SQLColumnInfoVectorIterator it = selectInfo.columns.begin();
	for (; it != selectInfo.columns.end(); ++it) {
		SQLColumnInfo *columnInfo = *it;

		// set SQLColumnInfo::tableInfo and SQLTableInfo::columnList.
		if (selectInfo.tables.size() == 1) {
			SQLTableInfo *tableInfo = *selectInfo.tables.begin();
			if (columnInfo->tableVar.empty())
				columnInfo->associate(tableInfo);
			else if (columnInfo->tableVar == tableInfo->varName)
				columnInfo->associate(tableInfo);
			else {
				MLPL_DBG("columnInfo.tableVar (%s) != "
				         "tableInfo.varName (%s)\n",
				         columnInfo->tableVar.c_str(),
				         tableInfo->varName.c_str());
				return false;
			}
			continue;
		}

		const SQLTableInfo *tableInfo
		  = getTableInfoFromVarName(selectInfo, columnInfo->tableVar);
		if (!tableInfo)
			return false;
		columnInfo->associate(const_cast<SQLTableInfo *>(tableInfo));

		// set SQLColumnInfo::baseDef.
	}
	return true;
}

bool SQLProcessor::associateTableWithStaticInfo(SQLSelectInfo &selectInfo)
{
	SQLTableInfoVectorIterator tblInfoIt = selectInfo.tables.begin();
	for (; tblInfoIt != selectInfo.tables.end(); ++tblInfoIt) {
		SQLTableInfo *tableInfo = *tblInfoIt;
		TableNameStaticInfoMapIterator it;
		it = m_tableNameStaticInfoMap.find(tableInfo->name);
		if (it == m_tableNameStaticInfoMap.end()) {
			MLPL_DBG("Not found table: %s\n",
			         tableInfo->name.c_str());
			return false;
		}
		const SQLTableStaticInfo *staticInfo = it->second;
		tableInfo->staticInfo = staticInfo;
	}
	return true;
}

bool
SQLProcessor::setColumnTypeAndBaseDefInColumnInfo(SQLSelectInfo &selectInfo)
{
	SQLColumnInfoVectorIterator it = selectInfo.columns.begin();
	for (; it != selectInfo.columns.end(); ++it) {
		SQLColumnInfo *columnInfo = *it;

		// columnType
		columnInfo->setColumnType();

		// baseDef
		if (columnInfo->columnType == SQLColumnInfo::COLUMN_TYPE_ALL)
			continue;

		if (!columnInfo->tableInfo) {
			MLPL_BUG("columnInfo->stableInfo is NULL\n");
			return false;
		}

		const SQLTableStaticInfo *staticInfo =
		  columnInfo->tableInfo->staticInfo;
		if (!staticInfo) {
			MLPL_BUG("staticInfo is NULL\n");
			return false;
		}

		columnInfo->columnBaseDef =
		  getColumnBaseDefinitionFromColumnName(columnInfo->tableInfo,
		                                        columnInfo->baseName);
		if (!columnInfo->columnBaseDef)
			return false;
	}
	return true;
}

void SQLProcessor::addColumnDefs(SQLSelectInfo &selectInfo,
                                 const SQLTableInfo &tableInfo,
                                 const ColumnBaseDefinition &columnBaseDef)
{
	selectInfo.columnDefs.push_back(SQLColumnDefinition());
	SQLColumnDefinition &colDef = selectInfo.columnDefs.back();
	colDef.columnBaseDef = &columnBaseDef;
	colDef.tableInfo     = &tableInfo;
	colDef.schema        = getDBName();
	colDef.table         = tableInfo.name;
	colDef.tableVar      = tableInfo.varName;
	colDef.column        = columnBaseDef.columnName;
	colDef.columnVar     = columnBaseDef.columnName;
}

bool SQLProcessor::addAllColumnDefs(SQLSelectInfo &selectInfo,
                                    const SQLTableInfo &tableInfo)
{
	if (!tableInfo.staticInfo) {
		MLPL_BUG("tableInfo.staticInfo is NULL\n");
		return false;
	}

	ColumnBaseDefListConstIterator it;
	it = tableInfo.staticInfo->columnBaseDefList.begin();
	for (; it != tableInfo.staticInfo->columnBaseDefList.end(); ++it) {
		const ColumnBaseDefinition &columnBaseDef = *it;
		addColumnDefs(selectInfo, tableInfo, columnBaseDef);
	}
	return true;
}

bool SQLProcessor::makeColumnDefs(SQLSelectInfo &selectInfo)
{
	SQLColumnInfoVectorIterator it = selectInfo.columns.begin();
	for (; it != selectInfo.columns.end(); ++it) {
		SQLColumnInfo *columnInfo = *it;
		int columnType = columnInfo->columnType;
		if (!columnInfo->tableInfo) {
			MLPL_BUG("columnInfo->tableInfo is NULL\n");
			return false;
		}
		if (columnType == SQLColumnInfo::COLUMN_TYPE_ALL) {
			if (!addAllColumnDefs(selectInfo,
			                      *columnInfo->tableInfo)) {
				return false;
			}
		} else if (columnType == SQLColumnInfo::COLUMN_TYPE_NORMAL) {
			if (!columnInfo->columnBaseDef) {
				MLPL_BUG("columnInfo.columnBaseDef is NULL\n");
				return false;
			}
			addColumnDefs(selectInfo,
			              *columnInfo->tableInfo,
			              *columnInfo->columnBaseDef);
		} else {
			MLPL_BUG("Invalid columnType: %d\n", columnType);
			return false;
		}
	}
	return true;
}

bool SQLProcessor::makeItemTables(SQLSelectInfo &selectInfo)
{
	SQLTableInfoVectorIterator tblInfoIt = selectInfo.tables.begin();
	for (; tblInfoIt != selectInfo.tables.end(); ++tblInfoIt) {
		const SQLTableInfo *tableInfo = *tblInfoIt;
		SQLTableMakeFunc func = tableInfo->staticInfo->tableMakeFunc;
		ItemTablePtr tablePtr = (this->*func)(selectInfo, *tableInfo);
		if (!tablePtr.hasData()) {
			MLPL_DBG("ItemTable: empty.\n");
			return false;
		}
		selectInfo.itemTablePtrList.push_back(tablePtr);
	}
	return true;
}

bool SQLProcessor::doJoin(SQLSelectInfo &selectInfo)
{
	if (selectInfo.itemTablePtrList.size() == 1) {
		selectInfo.joinedTable = *selectInfo.itemTablePtrList.begin();
		return true;
	}

	ItemTablePtrListConstIterator it = selectInfo.itemTablePtrList.begin();
	for (; it != selectInfo.itemTablePtrList.end(); ++it) {
		const ItemTablePtr &tablePtr = *it;
		selectInfo.joinedTable = crossJoin(selectInfo.joinedTable,
		                                   tablePtr);
	}
	return true;
}

bool SQLProcessor::fixupWhereColumn(SQLSelectInfo &selectInfo)
{
	for (size_t i = 0; i < selectInfo.whereColumnVector.size(); i++) {
		SQLWhereColumn *whereColumn = selectInfo.whereColumnVector[i];

		const string &columnName = whereColumn->getColumnName();
		string baseName;
		string tableVar;
		if (!parseColumnName(columnName, baseName, tableVar))
			return false;

		// find TableInfo in which the column should be contained
		const SQLTableInfo *tableInfo;
		if (selectInfo.tables.size() == 1) {
			tableInfo = *selectInfo.tables.begin();
		} else {
			tableInfo = getTableInfoFromVarName(selectInfo,
                                                            tableVar);
		}
		if (!tableInfo) {
			MLPL_DBG("Failed to find TableInfo: %s\n",
			         columnName.c_str());
			return false;
		}

		ColumnBaseDefinition *columnBaseDef = 
		  getColumnBaseDefinitionFromColumnName(tableInfo, baseName);
		if (!columnBaseDef)
			return false;

		// set ItemId
		void *priv = whereColumn->getPrivateData();
		WhereColumnArg *arg = static_cast<WhereColumnArg *>(priv);
		arg->itemId = columnBaseDef->itemId;
	}
	return true;
}

bool SQLProcessor::selectMatchingRows(SQLSelectInfo &selectInfo)
{
	if (selectInfo.rootWhereElem->isEmpty()) {
		selectInfo.selectedTable = selectInfo.joinedTable;
		return true;
	}
	return selectInfo.joinedTable->foreach<SQLSelectInfo&>
	                                    (pickupMatchingRows, selectInfo);
}

bool SQLProcessor::pickupMatchingRows(const ItemGroup *itemGroup,
                                      SQLSelectInfo &selectInfo)
{
	ItemGroup *nonConstItemGroup = const_cast<ItemGroup *>(itemGroup);
	selectInfo.evalTargetItemGroup = nonConstItemGroup;
	bool result = selectInfo.rootWhereElem->evaluate();
	if (!result)
		return true;;
	selectInfo.selectedTable->add(nonConstItemGroup);
	return true;
}

bool SQLProcessor::packRequiredColumns(const ItemGroup *itemGroup,
                                       SQLSelectInfo &selectInfo)
{
	ItemGroup *grp = selectInfo.packedTable->addNewGroup();
	for (size_t i = 0; i < selectInfo.columnDefs.size(); i++) {
		ItemData *item;
		const SQLColumnDefinition &colDef = selectInfo.columnDefs[i];
		item = itemGroup->getItem(colDef.columnBaseDef->itemId);
		if (!item) {
			string msg;
			TRMSG(msg, "Failed to get ItemData: %"PRIu_ITEM"\n",
			      colDef.columnBaseDef->itemId);
			throw logic_error(msg);
		}
		grp->add(item);
	}
	return true;
}

bool SQLProcessor::makeTextRows(const ItemGroup *itemGroup,
                                SQLSelectInfo &selectInfo)
{
	selectInfo.textRows.push_back(StringVector());
	StringVector &textVector = selectInfo.textRows.back();
	for (size_t i = 0; i < selectInfo.columnDefs.size(); i++) {
		const SQLColumnDefinition &colDef = selectInfo.columnDefs[i];
		const ItemData *item =
		  itemGroup->getItem(colDef.columnBaseDef->itemId);
		if (!item) {
			MLPL_BUG("Failed to get ItemData: %"PRIu_ITEM"\n",
			         colDef.columnBaseDef->itemId);
			return false;
		}
		textVector.push_back(item->getString());
	}
	return true;
}

//
// Select status parsers
//
bool SQLProcessor::parseSectionFrom(SelectParserContext &ctx)
{
	ctx.selectInfo.columnParser.flush();
	ctx.section = SELECT_PARSING_SECTION_FROM;
	m_separatorCountSpaceComma.resetCounter();
	return true;
}

bool SQLProcessor::parseSectionWhere(SelectParserContext &ctx)
{
	ctx.section = SELECT_PARSING_SECTION_WHERE;
	return true;
}

bool SQLProcessor::parseSectionOrder(SelectParserContext &ctx)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(ctx, &currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		ctx.selectInfo.query.setParsingPosition(currPos);
		return false;
	}

	ctx.section = SELECT_PARSING_SECTION_ORDER_BY;
	return true;
}

bool SQLProcessor::parseSectionGroup(SelectParserContext &ctx)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(ctx, &currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		ctx.selectInfo.query.setParsingPosition(currPos);
		return false;
	}

	ctx.section = SELECT_PARSING_SECTION_GROUP_BY;
	return true;
}

bool SQLProcessor::parseSectionLimit(SelectParserContext &ctx)
{
	ctx.section = SELECT_PARSING_SECTION_LIMIT;
	return true;
}

//
// Select statment parsers
//
bool SQLProcessor::parseSelectedColumns(SelectParserContext &ctx)
{
	return ctx.selectInfo.columnParser.add(ctx.currWord, ctx.currWordLower);
	/*
	SQLColumnInfo *columnInfo = new SQLColumnInfo();
	ctx.selectInfo.columns.push_back(columnInfo);
	columnInfo->name = ctx.currWord;
	return true;

	return parseColumnName(columnInfo->name,
	                       columnInfo->baseName, columnInfo->tableVar);
	*/
}

bool SQLProcessor::parseGroupBy(SelectParserContext &ctx)
{
	MLPL_BUG("Not implemented: GROUP_BY\n");
	return false;
}

bool SQLProcessor::parseFrom(SelectParserContext &ctx)
{
	bool isTableName = true;
	if (ctx.indexInTheStatus > 0) {
		int commaCount = m_separatorCountSpaceComma.getCount(',');
		if (commaCount == 0)
			isTableName = false;
		else if (commaCount == 1)
			isTableName = true;
		else {
			string msg;
			TRMSG(msg, "commaCount: %d.", commaCount);
			throw logic_error(msg);
		}
		m_separatorCountSpaceComma.resetCounter();
	}

	if (isTableName) {
		SQLTableInfo *tableInfo = new SQLTableInfo();
		ctx.selectInfo.tables.push_back(tableInfo);
		tableInfo->name = ctx.currWord;
		return true;
	}

	if (ctx.selectInfo.tables.empty()) {
		MLPL_DBG("selectInfo.tables is empty\n");
		return false;
	}

	SQLTableInfo *tableInfo = ctx.selectInfo.tables.back();
	tableInfo->varName = ctx.currWord;

	pair<SQLTableVarNameInfoMapIterator, bool> ret =
	  ctx.selectInfo.tableVarInfoMap.insert
	    (pair<string, SQLTableInfo *>(tableInfo->varName, tableInfo));
	if (!ret.second) {
		string msg;
		TRMSG(msg, "Failed to insert: table name: %s, %s.",
		      tableInfo->varName.c_str(),
		      ctx.selectInfo.query.getString());
		throw msg;
	}
	return true;
}

bool SQLProcessor::parseWhere(SelectParserContext &ctx)
{
	bool doKeywordCheck = true;
	bool currWordString = false;
	if (ctx.quotOpen) {
		ctx.quotOpen = false;
		doKeywordCheck = false;
		currWordString = true;
	} else if (ctx.betweenStep != BETWEEN_NONE) {
		return parseWhereBetween(ctx);
	}

	// check if this is the keyword
	if (doKeywordCheck) {
		map<string, SelectSubParser>::iterator it;
		it = m_whereKeywordHandlerMap.find(ctx.currWordLower);
		if (it != m_whereKeywordHandlerMap.end()) {
			SelectSubParser subParser = NULL;
			subParser = it->second;
			return (this->*subParser)(ctx);
		}
	}

	// parse word as the hand
	SQLWhereElement *whereElem = ctx.selectInfo.currWhereElem;
	bool shouldLeftHand = !(whereElem->getLeftHand());

	SQLWhereElement *handElem = NULL;
	if (currWordString)
		handElem = new SQLWhereString(ctx.currWord);
	else if (shouldLeftHand)
		handElem = createSQLWhereColumn(ctx);
	else {
		PolytypeNumber ptNum(ctx.currWord);
		if (ptNum.getType() != PolytypeNumber::TYPE_NONE)
			handElem = new SQLWhereNumber(ptNum);
		else
			handElem = createSQLWhereColumn(ctx);
	}

	if (shouldLeftHand)
		whereElem->setLeftHand(handElem);
	else if (!whereElem->getRightHand())
		whereElem->setRightHand(handElem);
	else {
		string treeInfo;
		ctx.selectInfo.rootWhereElem->getTreeInfo(treeInfo, 256);
		string msg;
		TRMSG(msg, "Both hands of currWhereElem are not NULL: %s\n%s",
		      ctx.currWord.c_str(), treeInfo.c_str());
		throw logic_error(msg);
	}
	return true;
}

bool SQLProcessor::parseOrderBy(SelectParserContext &ctx)
{
	ctx.selectInfo.orderedColumns.push_back(ctx.currWord);
	return true;
}

bool SQLProcessor::parseLimit(SelectParserContext &ctx)
{
	MLPL_BUG("Not implemented: %s: %s\n", ctx.currWord.c_str(), __func__);
	return true;
}

//
// Sub statement parsers
//
bool SQLProcessor::parseWhereBetween(SelectParserContext &ctx)
{
	string msg;

	// check
	SQLWhereOperator *op = ctx.selectInfo.currWhereElem->getOperator();
	if (!op) {
		TRMSG(msg, "ctx.currWhereElem->getOperator: NULL\n");
		throw logic_error(msg);
	}
	if (op->getType() != SQL_WHERE_OP_BETWEEN) {
		TRMSG(msg, "Operator is not SQL_WHERE_OP_BETWEEN: %d\n",
		      op->getType());
		throw logic_error(msg);
	}

	if (ctx.betweenStep == BETWEEN_EXPECT_FIRST) {
		ctx.betweenFirstValue = ctx.currWord;
		if (ctx.betweenFirstValue.getType()
		      == PolytypeNumber::TYPE_NONE) {
			MLPL_DBG("Unexpected value: %s", ctx.currWord.c_str());
			return false;
		}
		ctx.betweenStep = BETWEEN_EXPECT_AND;
	} else if (ctx.betweenStep == BETWEEN_EXPECT_AND) {
		if (ctx.currWordLower != "and") {
			MLPL_DBG("Unexpected value: %s", ctx.currWord.c_str());
			return false;
		}
		ctx.betweenStep = BETWEEN_EXPECT_SECOND;
	} else if (ctx.betweenStep == BETWEEN_EXPECT_SECOND) {
		PolytypeNumber secondValue = ctx.currWord;
		if (secondValue.getType() == PolytypeNumber::TYPE_NONE) {
			MLPL_DBG("Unexpected value: %s", ctx.currWord.c_str());
			return false;
		}
		SQLWhereElement *elem =
		  new SQLWherePairedNumber(ctx.betweenFirstValue, secondValue);
		ctx.selectInfo.currWhereElem->setRightHand(elem);
		ctx.betweenStep = BETWEEN_NONE;
	} else {
		TRMSG(msg, "Unexpected state: %d\n", ctx.betweenStep);
		throw logic_error(msg);
	}
	return true;
}

//
// Where section keyword handler
//
bool SQLProcessor::whereHandlerAnd(SelectParserContext &ctx)
{
	string msg;
	SQLWhereElement *currWhereElem = ctx.selectInfo.currWhereElem;
	if (!currWhereElem->isFull()) {
		TRMSG(msg, "currWhereElem is not full.");
		throw logic_error(msg);
	}

	// create new element 
	SQLWhereElement *andWhereElem = new SQLWhereElement();
	andWhereElem->setOperator(new SQLWhereOperatorAnd());

	// insert the created element
	SQLWhereElement *insertPair =
	  currWhereElem->findInsertPoint(andWhereElem);
	if (insertPair == NULL) {
		andWhereElem->setLeftHand(ctx.selectInfo.rootWhereElem);
		ctx.selectInfo.rootWhereElem = andWhereElem;
	} else {
		if (insertPair->getRightHand() != currWhereElem) {
			TRMSG(msg, "insertPair RHS != currWhereElem.");
			throw logic_error(msg);
		}
		andWhereElem->setLeftHand(currWhereElem);
		insertPair->setRightHand(andWhereElem);
	}

	// set the newly created right hand element
	SQLWhereElement *newRightElem = new SQLWhereElement();
	andWhereElem->setRightHand(newRightElem);

	// set the current element
	ctx.selectInfo.currWhereElem = newRightElem;

	return true;
}

bool SQLProcessor::whereHandlerBetween(SelectParserContext &ctx)
{
	SQLWhereOperator *opBetween = new SQLWhereOperatorBetween();
	ctx.selectInfo.currWhereElem->setOperator(opBetween);
	ctx.betweenStep = BETWEEN_EXPECT_FIRST;
	return true;
}

//
// Callbacks for parsing 'where' section
//
void SQLProcessor::whereCbEq(const char separator, SelectParserContext *ctx)
{
	SQLWhereOperator *opEq = new SQLWhereOperatorEqual();
	ctx->selectInfo.currWhereElem->setOperator(opEq);
}

void SQLProcessor::whereCbQuot(const char separator, SelectParserContext *ctx)
{
	SeparatorCheckerWithCallback &separatorCBForWhere
	  = ctx->sqlProcessor->m_separatorCBForWhere;
	if (ctx->quotOpen) {
		separatorCBForWhere.unsetAlternative();
		return;
	}
	ctx->quotOpen = true;
	separatorCBForWhere.setAlternative(&ParsableString::SEPARATOR_QUOT);
}

//
// General sub routines
//
string SQLProcessor::readNextWord(SelectParserContext &ctx,
                                  ParsingPosition *position)
{
	SeparatorChecker *separator = m_selectSeprators[ctx.section];
	if (position)
		*position = ctx.selectInfo.query.getParsingPosition();
	return ctx.selectInfo.query.readWord(*separator);
}

SQLWhereColumn *SQLProcessor::createSQLWhereColumn(SelectParserContext &ctx)
{
	WhereColumnArg *arg = new WhereColumnArg(ctx.selectInfo);
	SQLWhereColumn *whereColumn =
	  new SQLWhereColumn(ctx.currWord, whereColumnDataGetter,
	                     arg, wereColumnPrivDataDestructor);
	ctx.selectInfo.whereColumnVector.push_back(whereColumn);
	return whereColumn;
}

bool SQLProcessor::parseColumnName(const string &name,
                                   string &baseName, string &tableVar)
{
	size_t dotPos = name.find('.');
	if (dotPos == 0) {
		MLPL_DBG("Column name begins from dot. : %s", name.c_str());
		return false;
	}
	if (dotPos == (name.size() - 1)) {
		MLPL_DBG("Column name ends with dot. : %s", name.c_str());
		return false;
	}

	if (dotPos != string::npos) {
		tableVar = string(name, 0, dotPos);
		baseName = string(name, dotPos + 1);
	} else {
		baseName = name;
	}
	return true;
}

ColumnBaseDefinition *
SQLProcessor::getColumnBaseDefinitionFromColumnName
  (const SQLTableInfo *tableInfo, string &baseName)
{
	const SQLTableStaticInfo *staticInfo = tableInfo->staticInfo;
	if (!staticInfo) {
		string msg;
		TRMSG(msg, "staticInfo is NULL\n");
		throw logic_error(msg);
	}

	ItemNameColumnBaseDefRefMapConstIterator it;
	it = staticInfo->columnBaseDefMap.find(baseName);
	if (it == staticInfo->columnBaseDefMap.end()) {
		MLPL_DBG("Not found column: %s\n", baseName.c_str());
		return NULL;
	}
	return it->second;
}

const SQLTableInfo *
SQLProcessor::getTableInfoFromVarName(SQLSelectInfo &selectInfo,
                                      string &tableVar)
{
	map<string, const SQLTableInfo *>::iterator it;
	it = selectInfo.tableVarInfoMap.find(tableVar);
	if (it == selectInfo.tableVarInfoMap.end()) {
		MLPL_DBG("Failed to find: %s\n", tableVar.c_str());
		return NULL;
	}
	return it->second;
}

ItemDataPtr
SQLProcessor::whereColumnDataGetter(SQLWhereColumn *whreColumn, void *priv)
{
	WhereColumnArg *arg = static_cast<WhereColumnArg *>(priv);
	SQLSelectInfo &selectInfo = arg->selectInfo;
	ItemId itemId = arg->itemId;
	return ItemDataPtr(selectInfo.evalTargetItemGroup->getItem(itemId));
}

void SQLProcessor::wereColumnPrivDataDestructor
  (SQLWhereColumn *whereColumn, void *priv)
{
	WhereColumnArg *arg = static_cast<WhereColumnArg *>(priv);
	delete arg;
}
