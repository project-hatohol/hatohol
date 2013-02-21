#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cppcutter.h>
#include "SQLFromParser.h"
#include "Asura.h"
#include "AsuraException.h"

namespace testSQLFromParser {

static void _assertInputStatement(SQLFromParser &fromParser,
                                  ParsableString &statement)
{
	SeparatorCheckerWithCallback *separator =
	  fromParser.getSeparatorChecker();

	try {
		while (!statement.finished()) {
			string word = statement.readWord(*separator);
			string lower = StringUtils::toLower(word);
			fromParser.add(word, lower);
		}
		fromParser.close();
	} catch (const AsuraException &e) {
		cut_fail("Exception: <%s:%d> %s\n",
		         e.getSourceFileName().c_str(), e.getLineNumber(),
		         e.what());
	}
}
#define assertInputStatement(P,S) cut_trace(_assertInputStatement(P,S))

#define DEFINE_PARSER_AND_RUN(PARSER, TABLE_FORMULA, STATEMENT) \
ParsableString _statement(STATEMENT); \
SQLFromParser PARSER; \
assertInputStatement(PARSER, _statement); \
SQLTableFormula *TABLE_FORMULA = PARSER.getTableFormula(); \
cppcut_assert_not_null(TABLE_FORMULA);

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_oneTable(void)
{
	const char *tableName = "tab";
	string statement = StringUtils::sprintf("from %s", tableName);
	DEFINE_PARSER_AND_RUN(fromParser, tableFormula, statement);
}

} // namespace testSQLFromParser
