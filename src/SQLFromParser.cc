/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SQLFromParser.h"
#include "FormulaOperator.h"
#include "ItemDataUtils.h"
#include "SQLProcessorException.h"
#include "SQLUtils.h"

SQLFromParser::SubParser SQLFromParser::m_subParsers[] = {
	&SQLFromParser::subParserExpectTableName,
	&SQLFromParser::subParserPostTableName,
	&SQLFromParser::subParserCreatedTable,
	&SQLFromParser::subParserGotInner,
	&SQLFromParser::subParserExpectOn,
	&SQLFromParser::subParserExpectLeftField,
	&SQLFromParser::subParserExpectJoinEqual,
	&SQLFromParser::subParserExpectRightField,
};
size_t SQLFromParser::m_numSubParsers =
  sizeof(m_subParsers) /  sizeof(SQLFromParser::SubParser);

struct SQLFromParser::PrivateContext {

	ParsingState     state;
	SQLTableFormula *tableFormula;
	SQLTableElementList tableElementList;
	string           tableName;
	bool             onParsingInnerJoin;
	SQLTableElement *rightTableOfInnerJoin;
	string           innerJoinLeftTableName;
	string           innerJoinLeftColumnName;
	string           innerJoinRightTableName;
	string           innerJoinRightColumnName;
	SQLColumnIndexResoveler *columnIndexResolver;

	// variables used when join calculation
	bool             existsMode;
	ItemTablePtr     joinedTable;

	// constructor
	PrivateContext(void)
	: state(PARSING_STAT_EXPECT_TABLE_NAME),
	  tableFormula(NULL),
	  onParsingInnerJoin(false),
	  rightTableOfInnerJoin(NULL),
	  columnIndexResolver(NULL),
	  existsMode(false)
	{
	}

	~PrivateContext()
	{
		if (tableFormula)
			delete tableFormula;
		if (rightTableOfInnerJoin)
			delete rightTableOfInnerJoin;
	}

	void clearInnerJoinParts(void)
	{
		rightTableOfInnerJoin = NULL;
		innerJoinLeftTableName.clear();
		innerJoinLeftColumnName.clear();
		innerJoinRightTableName.clear();
		innerJoinRightColumnName.clear();
		existsMode = false;
	}

};

// ---------------------------------------------------------------------------
// Public methods (static)
// ---------------------------------------------------------------------------
void SQLFromParser::init(void)
{
	int m_numSubParsers = sizeof(m_subParsers) /  sizeof(SubParser);
	if (m_numSubParsers != NUM_PARSING_STAT) {
		THROW_ASURA_EXCEPTION(
		  "The number of m_numSubParsers is wrong: "
		  "expected/acutual: %zd/%zd",
		  NUM_PARSING_STAT, m_numSubParsers);
	}
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLFromParser::SQLFromParser(void)
: m_ctx(NULL),
  m_separator(" =,")
{
	m_ctx = new PrivateContext();
	m_separator.setCallbackTempl<SQLFromParser>
	  ('=', _separatorCbEqual, this);
	m_separator.setCallbackTempl<SQLFromParser>
	  (',', _separatorCbComma, this);
}

SQLFromParser::~SQLFromParser()
{
	if (m_ctx)
		delete m_ctx;
}

SQLTableFormula *SQLFromParser::getTableFormula(void) const
{
	return m_ctx->tableFormula;
}

SQLTableElementList &SQLFromParser::getTableElementList(void) const
{
	return m_ctx->tableElementList;
}

SeparatorCheckerWithCallback *SQLFromParser::getSeparatorChecker(void)
{
	return &m_separator;
}

void
SQLFromParser::setColumnIndexResolver(SQLColumnIndexResoveler *resolver)
{
	m_ctx->columnIndexResolver = resolver;
}

void SQLFromParser::prepareJoin(
  const ColumnComparisonInfoList &columnComparisonInfoList,
  SQLTableProcessContextIndex *ctxIndex)
{
	// make SQLTableProcessContext
	SQLTableElementListIterator it = m_ctx->tableElementList.begin();
	for (; it != m_ctx->tableElementList.end(); ++it) {
		SQLTableProcessContext *tableCtx = new SQLTableProcessContext();
		tableCtx->id = ctxIndex->tableCtxVector.size();
		SQLTableElement *tableElement = *it;
		tableElement->setSQLTableProcessContext(tableCtx);

		tableCtx->tableElement = tableElement;
		const string &name = tableElement->getName();
		ctxIndex->tableNameCtxMap[name] = tableCtx;
		const string &var = tableElement->getVarName();
		if (!var.empty())
			ctxIndex->tableVarCtxMap[var] = tableCtx;

		ctxIndex->tableCtxVector.push_back(tableCtx);
	}

	// associate column comparison info list with SQLTableProcessorContext
	associateColumnComparisonInfoWithTableProcessorContext
	  (columnComparisonInfoList, ctxIndex);

	// call prepration function for each SQLTableFormula.
	m_ctx->tableFormula->prepareJoin(ctxIndex);
}

ItemTablePtr SQLFromParser::doJoin(FormulaElement *whereFormula,
                                   bool existsMode)
{
	// execute join loop
	m_ctx->existsMode = existsMode;
	IterateTableRowForJoin(m_ctx->tableElementList.begin(), whereFormula);
	return m_ctx->joinedTable;
}

void SQLFromParser::add(const string &word, const string &wordLower)
{
	if (m_ctx->state >= NUM_PARSING_STAT)
		THROW_ASURA_EXCEPTION( "Invalid state: %d", m_ctx->state);
	SubParser subParser = m_subParsers[m_ctx->state];
	(this->*subParser)(word, wordLower);
}

void SQLFromParser::flush(void)
{
	if (m_ctx->tableName.empty())
		return;

	if (m_ctx->state == PARSING_STAT_POST_TABLE_NAME) {
		string &tableName = m_ctx->tableName;
		makeTableElement(tableName);
		m_ctx->tableName.clear();
	}
}

void SQLFromParser::close(void)
{
	flush();

	if (m_ctx->state != PARSING_STAT_CREATED_TABLE) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Invaid status on close: %d", m_ctx->state);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

//
// Sub parsers
//
void SQLFromParser::subParserExpectTableName
  (const string &word, const string &wordLower)
{
	m_ctx->tableName = word;
	m_ctx->state = PARSING_STAT_POST_TABLE_NAME;
}

void SQLFromParser::subParserPostTableName
  (const string &word, const string &wordLower)
{
	if (wordLower == "inner") {
		flush();
		m_ctx->state = PARSING_STAT_GOT_INNER;
	} else if (wordLower == "on") {
		flush();
		m_ctx->state = PARSING_STAT_EXPECT_INNER_JOIN_LEFT_FIELD;
	} else {
		string &tableName = m_ctx->tableName;
		makeTableElement(tableName, word);
		m_ctx->tableName.clear();
	}
}

void SQLFromParser::subParserCreatedTable
  (const string &word, const string &wordLower)
{
	if (wordLower == "inner") {
		m_ctx->state = PARSING_STAT_GOT_INNER;
		return;
	}
}

void SQLFromParser::subParserGotInner
  (const string &word, const string &wordLower)
{
	if (wordLower == "join") {
		m_ctx->onParsingInnerJoin = true;
		m_ctx->state = PARSING_STAT_EXPECT_TABLE_NAME;
		return;
	}
}

void SQLFromParser::subParserExpectOn
  (const string &word, const string &wordLower)
{
	goNextStateIfWordIsExpected(
	  "on", wordLower, PARSING_STAT_EXPECT_INNER_JOIN_LEFT_FIELD);
}

void SQLFromParser::subParserExpectLeftField
  (const string &word, const string &wordLower)
{
	parseInnerJoinLeftField(word);
}

void SQLFromParser::subParserExpectJoinEqual
  (const string &word, const string &wordLower)
{
	THROW_SQL_PROCESSOR_EXCEPTION(
	  "Expected '=', but got: %s", word.c_str());
}

void SQLFromParser::subParserExpectRightField
  (const string &word, const string &wordLower)
{
	parseInnerJoinRightField(word);
}

void SQLFromParser::doJoineOneRow(FormulaElement *whereFormula)
{
	ItemGroupPtr activeRow = m_ctx->tableFormula->getActiveRow();
	if (!activeRow.hasData())
		return;

	bool shouldAdd = false;
	if (whereFormula == NULL) {
		// This condition occurs when no where section.
		shouldAdd = true;
	} else {
		ItemDataPtr matched = whereFormula->evaluate();
		if (!matched.hasData()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "The evaluated result of the where conditon "
			  "has no data.");
		}
		shouldAdd = *matched;
	}
	if (!shouldAdd)
		return;
	m_ctx->joinedTable->add(activeRow);
	if (m_ctx->existsMode)
		throw SQLFoundRowOnJoinException();
}

void SQLFromParser::IterateTableRowForJoin(SQLTableElementListIterator tableItr,
                                           FormulaElement *whereFormula)
{
	SQLTableElement *tableElement = *tableItr;
	SQLTableElementListIterator nextTableItr = tableItr;
	++nextTableItr;
	bool lastTable = (nextTableItr == m_ctx->tableElementList.end());

	tableElement->startRowIterator();
	while (!tableElement->rowIteratorEnd()) {
		if (!lastTable)
			IterateTableRowForJoin(nextTableItr, whereFormula);
		else
			doJoineOneRow(whereFormula);
		tableElement->rowIteratorInc();
	}
}

//
// general sub routines
//
void SQLFromParser::goNextStateIfWordIsExpected(const string &expectedWord,
                                                const string &actualWord,
                                                ParsingState nextState)
{
	if (actualWord != expectedWord) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Expected: %s, but got: %s",
		  expectedWord.c_str(), actualWord.c_str());
	}
	m_ctx->state = nextState;
}

void SQLFromParser::insertTableFormula(SQLTableFormula *tableFormula)
{
	if (!m_ctx->tableFormula) {
		m_ctx->tableFormula = tableFormula;
		return;
	}

	SQLTableJoin *tableJoin =
	  dynamic_cast<SQLTableJoin *>(m_ctx->tableFormula);
	if (tableJoin) {
		SQLTableFormula *rightFormula = tableJoin->getRightFormula();
		if (rightFormula) {
			delete tableFormula;
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Rigth side of the cross join has already set.");
		}
		tableJoin->setRightFormula(tableFormula);
		return;
	}

	THROW_SQL_PROCESSOR_EXCEPTION(
	  "Stopped due to the unknown condition: %s", __PRETTY_FUNCTION__);
}

void SQLFromParser::makeTableElement(const string &tableName,
                                     const string &varName)
{
	SQLTableElement *tableElem =
	  new SQLTableElement(tableName, varName, m_ctx->columnIndexResolver);
	m_ctx->tableElementList.push_back(tableElem);
	if (m_ctx->onParsingInnerJoin) {
		m_ctx->rightTableOfInnerJoin = tableElem;
		m_ctx->state = PARSING_STAT_EXPECT_ON;
		return;
	}
	insertTableFormula(tableElem);
	m_ctx->state = PARSING_STAT_CREATED_TABLE;
}

void SQLFromParser::associateColumnComparisonInfoWithTableProcessorContext
  (const ColumnComparisonInfoList &columnComparisonInfoList,
   SQLTableProcessContextIndex *ctxIndex)
{
	// NOTE: Current implementation just overwrites equalBoundTablexCtx
	// and related variables when they have already been set. The latest
	// written one may not be the most effective indexes. So more smart
	// algorithm should be applied.

	ColumnComparisonInfoListConstIterator it =
	  columnComparisonInfoList.begin();
	for (; it != columnComparisonInfoList.end(); ++it) {
		const ColumnComparisonInfo *columnCompInfo = *it;
		SQLTableProcessContext *leftTableCtx =
		   ctxIndex->getTableContext(columnCompInfo->leftTableName);
		SQLTableProcessContext *rightTableCtx =
		   ctxIndex->getTableContext(columnCompInfo->rightTableName);
		if (leftTableCtx == NULL || rightTableCtx == NULL) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "getTableContext: NULL, %s.%s (%p), %s.%s (%p)",
			  columnCompInfo->leftTableName.c_str(),
			  columnCompInfo->leftColumnName.c_str(),
			  leftTableCtx,
			  columnCompInfo->rightTableName.c_str(),
			  columnCompInfo->rightColumnName.c_str(),
			  rightTableCtx);
		}

		SQLTableProcessContext *tableCtx0, *tableCtx1;
		const string *tableName0, *tableName1;
		const string *columnName0, *columnName1;
		if (leftTableCtx->id < rightTableCtx->id) {
			tableCtx0 = leftTableCtx;
			tableCtx1 = rightTableCtx;;
			tableName0 = &columnCompInfo->leftTableName;
			tableName1 = &columnCompInfo->rightTableName;
			columnName0 = &columnCompInfo->leftColumnName;
			columnName1 = &columnCompInfo->rightColumnName;
		} else {
			tableCtx0 = rightTableCtx;
			tableCtx1 = leftTableCtx;;
			tableName0 = &columnCompInfo->rightTableName;
			tableName1 = &columnCompInfo->leftTableName;
			columnName0 = &columnCompInfo->rightColumnName;
			columnName1 = &columnCompInfo->leftColumnName;
		}

		tableCtx1->equalBoundTableCtx = tableCtx0;
		tableCtx1->equalBoundColumnIndex =
		  m_ctx->columnIndexResolver->getIndex(*tableName0,
		                                       *columnName0);
		tableCtx1->equalBoundMyIndex =
		  m_ctx->columnIndexResolver->getIndex(*tableName1,
		                                       *columnName1);
	}
}

void SQLFromParser::makeCrossJoin(void)
{
	if (!m_ctx->tableFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "No table at left side in spite of attempting to "
		  "make a cross join element.");
	}
	SQLTableCrossJoin *crossJoin = new SQLTableCrossJoin();
	crossJoin->setLeftFormula(m_ctx->tableFormula);
	m_ctx->tableFormula = crossJoin;
	m_ctx->state = PARSING_STAT_EXPECT_TABLE_NAME;
}

void SQLFromParser::makeInnerJoin(void)
{
	if (!m_ctx->tableFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "No table at left side in spite of attempting to "
		  "make an inner join element.");
	}
	SQLTableInnerJoin *innerJoin =
	  new SQLTableInnerJoin(m_ctx->innerJoinLeftTableName,
	                        m_ctx->innerJoinLeftColumnName,
	                        m_ctx->innerJoinRightTableName,
	                        m_ctx->innerJoinRightColumnName,
	                        m_ctx->columnIndexResolver);
	innerJoin->setLeftFormula(m_ctx->tableFormula);
	m_ctx->tableFormula = innerJoin;
	innerJoin->setRightFormula(m_ctx->rightTableOfInnerJoin);
	m_ctx->clearInnerJoinParts();
	m_ctx->state = PARSING_STAT_EXPECT_TABLE_NAME;
}

void SQLFromParser::parseInnerJoinLeftField(const string &fieldName)
{
	SQLUtils::decomposeTableAndColumn(fieldName,
	                                  m_ctx->innerJoinLeftTableName,
	                                  m_ctx->innerJoinLeftColumnName);
	m_ctx->state = PARSING_STAT_EXPECT_INNER_JOIN_EQUAL;
}

void SQLFromParser::parseInnerJoinRightField(const string &fieldName)
{
	SQLUtils::decomposeTableAndColumn(fieldName,
	                                  m_ctx->innerJoinRightTableName,
	                                  m_ctx->innerJoinRightColumnName);
	makeInnerJoin();
	m_ctx->state = PARSING_STAT_CREATED_TABLE;
}

//
// SeparatorChecker callbacks
//
void SQLFromParser::_separatorCbEqual(const char separator,
                                       SQLFromParser *fromParsesr)
{
	fromParsesr->separatorCbEqual(separator);
}

void SQLFromParser::separatorCbEqual(const char separator)
{
	if (m_ctx->state != PARSING_STAT_EXPECT_INNER_JOIN_EQUAL) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Encountered an unexpectd equal. state: %d", m_ctx->state);
	}
	m_ctx->state = PARSING_STAT_EXPECT_INNER_JOIN_RIGHT_FIELD;
}

void SQLFromParser::_separatorCbComma(const char separator,
                                      SQLFromParser *fromParsesr)
{
	fromParsesr->separatorCbComma(separator);
}

void SQLFromParser::separatorCbComma(const char separator)
{
	if (m_ctx->state == PARSING_STAT_POST_TABLE_NAME)
		flush();
	if (m_ctx->state == PARSING_STAT_CREATED_TABLE) {
		makeCrossJoin();
		return;
	}

	THROW_SQL_PROCESSOR_EXCEPTION("Encountered an unexpectd comma.");
}
