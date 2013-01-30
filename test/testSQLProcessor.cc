#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <typeinfo>
#include <cutter.h>
#include <cppcutter.h>
#include "SQLProcessor.h"
#include "FormulaFunction.h"
#include "Asura.h"
#include "Utils.h"

namespace testSQLProcessor {

static const int TABLE_ID = 1;
static const int TABLE_ID_A = 2;

static const char *TABLE_NAME = "TestTable";
static const char *TABLE_NAME_A = "TestTableA";

static const char *COLUMN_NAME1 = "column1";
static const char *COLUMN_NAME2 = "column2";

static const char *COLUMN_NAME_A1 = "columnA1";
static const char *COLUMN_NAME_A2 = "columnA2";
static const char *COLUMN_NAME_A3 = "columnA3";

static const int NUM_COLUMN_DEFS = 2;
static ColumnBaseDefinition COLUMN_DEFS[NUM_COLUMN_DEFS] = {
  {0, TABLE_NAME, COLUMN_NAME1, SQL_COLUMN_TYPE_INT, 11, 0},
  {0, TABLE_NAME, COLUMN_NAME2, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

static const int NUM_COLUMN_DEFS_A = 3;
static ColumnBaseDefinition COLUMN_DEFS_A[NUM_COLUMN_DEFS_A] = {
  {0, TABLE_NAME_A, COLUMN_NAME_A1, SQL_COLUMN_TYPE_INT, 11, 0},
  {0, TABLE_NAME_A, COLUMN_NAME_A2, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
  {0, TABLE_NAME_A, COLUMN_NAME_A3, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

class TestSQLProcessor : public SQLProcessor {
public:
	TestSQLProcessor(void)
	: SQLProcessor(m_tableNameStaticInfoMap)
	{
		initStaticInfo();
	}

	// Implementation of abstruct method
	const char *getDBName(void) {
		return "TestTable";
	}

	void callParseSelectStatement(SQLSelectInfo &selectInfo) {
		parseSelectStatement(selectInfo);
	}

	const ItemTablePtr
	tableMakeFunc(SQLSelectInfo &selectInfo,
	              const SQLTableInfo &tableInfo)
	{
		const ItemTablePtr tablePtr;
		return tablePtr;
	}

private:
	TableNameStaticInfoMap m_tableNameStaticInfoMap;
	SQLTableStaticInfo     m_staticInfo[2];


	void initStaticInfoEach(SQLTableStaticInfo *staticInfo,
	                        int tableId, const char *tableName,
	                        int numColumnDefs,
	                        ColumnBaseDefinition *columnDefs)
	{
		m_tableNameStaticInfoMap[TABLE_NAME] = staticInfo;
		staticInfo->tableId = tableId;
		staticInfo->tableName = tableName;
		staticInfo->tableMakeFunc =
		  static_cast<SQLTableMakeFunc>
		  (&TestSQLProcessor::tableMakeFunc);

		ColumnBaseDefList &list =
		  const_cast<ColumnBaseDefList &>
		  (staticInfo->columnBaseDefList);

		ItemNameColumnBaseDefRefMap &map =
		  const_cast<ItemNameColumnBaseDefRefMap &>
		  (staticInfo->columnBaseDefMap);

		for (int i = 0; i < numColumnDefs; i++) {
			list.push_back(columnDefs[i]);
			map[columnDefs[i].columnName] = &columnDefs[i];
		}
	}

	void initStaticInfo(void) {
		initStaticInfoEach(&m_staticInfo[0], TABLE_ID, TABLE_NAME,
		                   NUM_COLUMN_DEFS, COLUMN_DEFS);
		initStaticInfoEach(&m_staticInfo[1], TABLE_ID_A, TABLE_NAME_A,
		                   NUM_COLUMN_DEFS_A, COLUMN_DEFS_A);
	}
};

static void
_assertWhereElementColumn(SQLWhereElement *whereElem, const char *expected)
{
	cut_assert_not_null(whereElem);
	cppcut_assert_equal(true, typeid(SQLWhereColumn) == typeid(*whereElem));
	SQLWhereColumn *elemColumn = dynamic_cast<SQLWhereColumn *>(whereElem);
	cppcut_assert_equal(string(expected), elemColumn->getColumnName());
}
#define assertWhereElementColumn(ELEM, EXPECT) \
cut_trace(_assertWhereElementColumn(ELEM, EXPECT))

static void
_assertWhereElementNumber(SQLWhereElement *whereElem, double expected)
{
	cut_assert_not_null(whereElem);
	cppcut_assert_equal(true, typeid(SQLWhereNumber) == typeid(*whereElem));
	SQLWhereNumber *elemNumber = dynamic_cast<SQLWhereNumber *>(whereElem);
	cppcut_assert_equal(expected, elemNumber->getValue().getAsDouble());
}
#define assertWhereElementNumber(ELEM, EXPECT) \
cut_trace(_assertWhereElementNumber(ELEM, EXPECT))

static void
_assertWhereElementString(SQLWhereElement *whereElem, const char *expected)
{
	cut_assert_not_null(whereElem);
	cppcut_assert_equal(true, typeid(SQLWhereString) == typeid(*whereElem));
	SQLWhereString *elemString = dynamic_cast<SQLWhereString *>(whereElem);
	cppcut_assert_equal(string(expected), elemString->getValue());
}
#define assertWhereElementString(ELEM, EXPECT) \
cut_trace(_assertWhereElementString(ELEM, EXPECT))

static void
_assertWhereElementPairedNumber(SQLWhereElement *whereElem,
                                double expect0, double expect1)
{
	cut_assert_not_null(whereElem);
	cppcut_assert_equal
	  (true, typeid(SQLWherePairedNumber) == typeid(*whereElem));
	SQLWherePairedNumber *elemPairedNumber
	  = dynamic_cast<SQLWherePairedNumber *>(whereElem);
	const PolytypeNumber &actual0 = elemPairedNumber->getFirstValue();
	const PolytypeNumber &actual1 = elemPairedNumber->getSecondValue();
	cppcut_assert_equal(expect0, actual0.getAsDouble());
	cppcut_assert_equal(expect1, actual1.getAsDouble());
}
#define assertWhereElementPairedNumber(ELEM, EXPECT0, EXPECT1) \
cut_trace(_assertWhereElementPairedNumber(ELEM, EXPECT0, EXPECT1))

static void
_assertWhereElementElement(SQLWhereElement *whereElem)
{
	cut_assert_not_null(whereElem);
	cppcut_assert_equal
	  (true, typeid(SQLWhereElement) == typeid(*whereElem),
	   cut_message("type: *whreElem: %s (%s)",
	               DEMANGLED_TYPE_NAME(*whereElem), TYPE_NAME(*whereElem)));
}
#define assertWhereElementElement(ELEM) \
cut_trace(_assertWhereElementElement(ELEM))

static void assertWhrereOperatorEqual(SQLWhereOperator *whereOp)
{
	cut_assert_not_null(whereOp);
	cppcut_assert_equal(true,
	                    typeid(SQLWhereOperatorEqual) == typeid(*whereOp));
}

static void _assertWhereOperatorEqual(SQLWhereElement *whereElem)
{
	cppcut_assert_equal(SQL_WHERE_ELEM_ELEMENT, whereElem->getType());
	cut_trace(assertWhrereOperatorEqual(whereElem->getOperator()));
}
#define assertWhereOperatorEqual(ELEM) \
cut_trace(_assertWhereOperatorEqual(ELEM))

static void assertWhrereOperatorAnd(SQLWhereOperator *whereOp)
{
	cut_assert_not_null(whereOp);
	cppcut_assert_equal
	  (true, typeid(SQLWhereOperatorAnd) == typeid(*whereOp),
	   cut_message("type: *whreOp: %s (%s)",
	               DEMANGLED_TYPE_NAME(*whereOp), TYPE_NAME(*whereOp)));
}

static void _assertWhereOperatorAnd(SQLWhereElement *whereElem)
{
	cppcut_assert_equal(SQL_WHERE_ELEM_ELEMENT, whereElem->getType());
	cut_trace(assertWhrereOperatorAnd(whereElem->getOperator()));
}
#define assertWhereOperatorAnd(ELEM) \
cut_trace(_assertWhereOperatorAnd(ELEM))

static void assertWhrereOperatorBetween(SQLWhereOperator *whereOp)
{
	cut_assert_not_null(whereOp);
	cppcut_assert_equal
	  (true, typeid(SQLWhereOperatorBetween) == typeid(*whereOp));
}

static void _assertWhereOperatorBetween(SQLWhereElement *whereElem)
{
	cppcut_assert_equal(SQL_WHERE_ELEM_ELEMENT, whereElem->getType());
	cut_trace(assertWhrereOperatorBetween(whereElem->getOperator()));
}
#define assertWhereOperatorBetween(ELEM) \
cut_trace(_assertWhereOperatorBetween(ELEM))


template<typename T>
static void assertFormulaElementType(FormulaElement *obj)
{
	cppcut_assert_equal
	  (true, typeid(T) == typeid(*obj),
	   cut_message("type: *obj: %s (%s)",
	               DEMANGLED_TYPE_NAME(*obj), TYPE_NAME(*obj)));
}

#define assertTypeFormulaColumn(P) \
cut_trace(assertFormulaElementType<FormulaColumn>(P))

#define assertFormulaFuncMax(P) \
cut_trace(assertFormulaElementType<FormulaFuncMax>(P))

static void _assertFormulaColumn(FormulaElement *elem, const char *expected)
{
	assertTypeFormulaColumn(elem);
	FormulaColumn *formulaColumn = dynamic_cast<FormulaColumn *>(elem);
	cut_assert_not_null(formulaColumn);
	cppcut_assert_equal(string(expected), formulaColumn->getName());
}
#define assertFormulaColumn(EL, EXP) cut_trace(_assertFormulaColumn(EL, EXP))

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_selectOneColumn(void)
{
	TestSQLProcessor proc;
	const char *columnName = "column";
	const char *tableName = TABLE_NAME;
	ParsableString parsable(
	  StringUtils::sprintf("%s from %s", columnName, tableName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	// column
	const SQLFormulaInfoVector formulaInfoVector
	  = selectInfo.columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());

	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(string(columnName), formulaInfo->expression);
	assertFormulaColumn(formulaInfo->formula, columnName);

	// table
	cut_assert_equal_int(1, selectInfo.tables.size());
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(tableName, (*table)->name.c_str());
}

void test_selectMultiColumn(void)
{
	TestSQLProcessor proc;
	const char *columns[] = {"c1", "a2", "b3", "z4", "x5", "y6"};
	const size_t numColumns = sizeof(columns) / sizeof(const char *);
	const char *tableName = TABLE_NAME;
	ParsableString parsable(
	  StringUtils::sprintf("%s,%s,%s, %s, %s, %s from %s",
	                       columns[0], columns[1], columns[2],
	                       columns[3], columns[4], columns[5],
	                       tableName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	// columns
	const SQLFormulaInfoVector formulaInfoVector
	  = selectInfo.columnParser.getFormulaInfoVector();
	cppcut_assert_equal(numColumns, formulaInfoVector.size());

	for (int idx = 0; idx < numColumns; idx++) {
		SQLFormulaInfo *formulaInfo = formulaInfoVector[idx];
		cppcut_assert_equal(string(columns[idx]),
		                    formulaInfo->expression);
		assertFormulaColumn(formulaInfo->formula, columns[idx]);
	}

	// table
	cut_assert_equal_int(1, selectInfo.tables.size());
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(tableName, (*table)->name.c_str());
}

void test_selectTableVar(void)
{
	TestSQLProcessor proc;
	const char *tableVarName = "tvar";
	ParsableString parsable(
	  StringUtils::sprintf("columnArg from %s %s",
	                       TABLE_NAME, tableVarName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);
	cut_assert_equal_int(1, selectInfo.tables.size());
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(tableVarName, (*table)->varName.c_str());
}

void test_selectOrderBy(void)
{
	TestSQLProcessor proc;
	const char *orderName = "orderArg";
	ParsableString parsable(
	  StringUtils::sprintf("column from %s  order by %s", TABLE_NAME,
	                       orderName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(1, selectInfo.orderedColumns.size());
	cut_assert_equal_string(orderName,
	                        selectInfo.orderedColumns[0].c_str());
}

void test_selectTwoTable(void)
{
	TestSQLProcessor proc;
	ParsableString parsable(
	  StringUtils::sprintf("* from %s,%s", TABLE_NAME, TABLE_NAME_A));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(2, selectInfo.tables.size());
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(TABLE_NAME, (*table)->name.c_str());
	++table;
	cut_assert_equal_string(TABLE_NAME_A, (*table)->name.c_str());
}

void test_selectTwoTableWithNames(void)
{
	const char var1[] = "var1";
	const char var2[] = "var2";
	TestSQLProcessor proc;
	ParsableString parsable(
	  StringUtils::sprintf("* from %s %s,%s %s",
	                       TABLE_NAME, var1, TABLE_NAME_A, var2));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(2, selectInfo.tables.size());
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(TABLE_NAME, (*table)->name.c_str());
	cut_assert_equal_string(var1, (*table)->varName.c_str());
	++table;
	cut_assert_equal_string(TABLE_NAME_A, (*table)->name.c_str());
	cut_assert_equal_string(var2, (*table)->varName.c_str());
}

void test_selectWhereEqNumber(void)
{
	TestSQLProcessor proc;
	const char *leftHand = "a";
	int rightHand = 1;
	ParsableString parsable(
	  StringUtils::sprintf("c1 from t1 where %s=%d", leftHand, rightHand));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	assertWhereElementColumn(selectInfo.rootWhereElem->getLeftHand(),
	                         leftHand);
	assertWhereElementNumber(selectInfo.rootWhereElem->getRightHand(),
	                         rightHand);
	assertWhereOperatorEqual(selectInfo.rootWhereElem);
}

void test_selectWhereEqString(void)
{
	TestSQLProcessor proc;
	const char *leftHand = "a";
	const char *rightHand = "foo XYZ";
	ParsableString parsable(
	  StringUtils::sprintf("c1 from t1 where %s='%s'",
	                       leftHand, rightHand));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	assertWhereElementColumn(selectInfo.rootWhereElem->getLeftHand(),
	                         leftHand);
	assertWhereElementString(selectInfo.rootWhereElem->getRightHand(),
	                         rightHand);
	assertWhereOperatorEqual(selectInfo.rootWhereElem);
}

void test_selectWhereEqColumnColumn(void)
{
	TestSQLProcessor proc;
	const char *leftHand = "a";
	const char *rightHand = "b";
	ParsableString parsable(
	  StringUtils::sprintf("c1 from t1 where %s=%s",
	                       leftHand, rightHand));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	assertWhereElementColumn(selectInfo.rootWhereElem->getLeftHand(),
	                         leftHand);
	assertWhereElementColumn(selectInfo.rootWhereElem->getRightHand(),
	                         rightHand);
	assertWhereOperatorEqual(selectInfo.rootWhereElem);
}

void test_selectWhereEqColumn(void)
{
	TestSQLProcessor proc;
	const char *leftHand = "a";
	const char *rightHand = "b";
	ParsableString parsable(
	  StringUtils::sprintf("c1 from t1 where %s=%s",
	                       leftHand, rightHand));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	assertWhereElementColumn(selectInfo.rootWhereElem->getLeftHand(),
	                         leftHand);
	assertWhereElementColumn(selectInfo.rootWhereElem->getRightHand(),
	                         rightHand);
	assertWhereOperatorEqual(selectInfo.rootWhereElem);
}

void test_selectWhereAnd(void)
{
	TestSQLProcessor proc;
	const char *leftHand0  = "a";
	int         rightHand0 = 1;
	const char *leftHand1  = "b";
	const char *rightHand1 = "foo";
	ParsableString parsable(
	  StringUtils::sprintf("c1 from t1 where %s=%d and %s='%s'",
	                       leftHand0, rightHand0, leftHand1, rightHand1));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	SQLWhereElement *leftElem = selectInfo.rootWhereElem->getLeftHand();
	SQLWhereElement *rightElem = selectInfo.rootWhereElem->getRightHand();
	assertWhereElementElement(leftElem);
	assertWhereElementElement(rightElem);
	assertWhereOperatorAnd(selectInfo.rootWhereElem);

	// left child
	assertWhereElementColumn(leftElem->getLeftHand(), leftHand0);
	assertWhereElementNumber(leftElem->getRightHand(), rightHand0);
	assertWhereOperatorEqual(leftElem);

	// right child
	assertWhereElementColumn(rightElem->getLeftHand(), leftHand1);
	assertWhereElementString(rightElem->getRightHand(), rightHand1);
	assertWhereOperatorEqual(rightElem);
}

void test_selectWhereBetween(void)
{
	TestSQLProcessor proc;
	const char *leftHand = "a";
	int v0 = 0;
	int v1 = 100;
	ParsableString parsable(
	  StringUtils::sprintf("c1 from t1 where %s between %d and %d",
	                       leftHand, v0, v1));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	SQLWhereElement *rootElem = selectInfo.rootWhereElem;
	assertWhereElementColumn(rootElem->getLeftHand(), leftHand);
	assertWhereElementPairedNumber(rootElem->getRightHand(), v0, v1);
	assertWhereOperatorBetween(rootElem);
}

void test_selectWhereIntAndStringAndInt(void)
{
	TestSQLProcessor proc;
	const char *leftHand0  = "a";
	int         rightHand0 = 1;
	const char *leftHand1  = "b";
	const char *rightHand1 = "foo";
	const char *leftHand2  = "c";
	int         rightHand2 = -5;
	ParsableString parsable(
	  StringUtils::sprintf("c1 from t1 where %s=%d and %s='%s' and %s=%d",
	                       leftHand0, rightHand0, leftHand1, rightHand1,
	                       leftHand2, rightHand2));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	SQLWhereElement *leftElem = selectInfo.rootWhereElem->getLeftHand();
	SQLWhereElement *rightElem = selectInfo.rootWhereElem->getRightHand();
	assertWhereElementElement(leftElem);
	assertWhereElementElement(rightElem);
	assertWhereOperatorAnd(selectInfo.rootWhereElem);

	// Top left child
	assertWhereElementColumn(leftElem->getLeftHand(), leftHand0);
	assertWhereElementNumber(leftElem->getRightHand(), rightHand0);
	assertWhereOperatorEqual(leftElem);

	// Top right child
	assertWhereElementElement(rightElem->getLeftHand());
	assertWhereElementElement(rightElem->getRightHand());
	assertWhereOperatorAnd(rightElem);

	// Second level left
	leftElem = rightElem->getLeftHand();
	assertWhereElementColumn(leftElem->getLeftHand(), leftHand1);
	assertWhereElementString(leftElem->getRightHand(), rightHand1);
	assertWhereOperatorEqual(leftElem);

	// Second level child
	rightElem = rightElem->getRightHand();
	assertWhereElementColumn(rightElem->getLeftHand(), leftHand2);
	assertWhereElementNumber(rightElem->getRightHand(), rightHand2);
	assertWhereOperatorEqual(rightElem);
}

void test_selectWhereIntAndColumnAndInt(void)
{
	TestSQLProcessor proc;
	const char *leftHand0  = "a";
	int         rightHand0 = 1;
	const char *leftHand1  = "b";
	const char *rightHand1 = "x";
	const char *leftHand2  = "c";
	int         rightHand2 = -5;
	ParsableString parsable(
	  StringUtils::sprintf("c1 from t1,t2 where %s=%d and %s=%s and %s=%d",
	                       leftHand0, rightHand0, leftHand1, rightHand1,
	                       leftHand2, rightHand2));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	SQLWhereElement *leftElem = selectInfo.rootWhereElem->getLeftHand();
	SQLWhereElement *rightElem = selectInfo.rootWhereElem->getRightHand();
	assertWhereElementElement(leftElem);
	assertWhereElementElement(rightElem);
	assertWhereOperatorAnd(selectInfo.rootWhereElem);

	// Top left child
	assertWhereElementColumn(leftElem->getLeftHand(), leftHand0);
	assertWhereElementNumber(leftElem->getRightHand(), rightHand0);
	assertWhereOperatorEqual(leftElem);

	// Top right child
	assertWhereElementElement(rightElem->getLeftHand());
	assertWhereElementElement(rightElem->getRightHand());
	assertWhereOperatorAnd(rightElem);

	// Second level left
	leftElem = rightElem->getLeftHand();
	assertWhereElementColumn(leftElem->getLeftHand(), leftHand1);
	assertWhereElementColumn(leftElem->getRightHand(), rightHand1);
	assertWhereOperatorEqual(leftElem);

	// Second level child
	rightElem = rightElem->getRightHand();
	assertWhereElementColumn(rightElem->getLeftHand(), leftHand2);
	assertWhereElementNumber(rightElem->getRightHand(), rightHand2);
	assertWhereOperatorEqual(rightElem);
}

void test_selectColumnElemMax(void)
{
	TestSQLProcessor proc;
	const char *columnName = "c1";
	ParsableString parsable(
	  StringUtils::sprintf("max(%s) from t1", columnName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	const SQLFormulaInfoVector formulaInfoVector
	  = selectInfo.columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	string expected = StringUtils::sprintf("max(%s)", columnName);
	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(expected, formulaInfo->expression);

	FormulaElement *formulaElem = formulaInfo->formula;
	assertFormulaFuncMax(formulaElem);
	FormulaFuncMax *formulaFuncMax
	  = dynamic_cast<FormulaFuncMax *>(formulaElem);
	cppcut_assert_equal((size_t)1, formulaFuncMax->getNumberOfArguments());
	FormulaElement *arg = formulaFuncMax->getArgument(0);
	assertFormulaColumn(arg, columnName);
}

} // namespace testSQLProcessor
