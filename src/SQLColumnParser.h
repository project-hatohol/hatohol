#ifndef SQLColumnParser_h
#define SQLColumnParser_h

#include <string>
#include <list>
#include <deque>
using namespace std;

#include "ParsableString.h"
using namespace mlpl;

#include "FormulaElement.h"
#include "FormulaFunction.h"

struct SQLColumnFormula {
	FormulaElement *formula;
	string          expression;
};

typedef vector<SQLColumnFormula *>       SQLColumnFormulaVector;
typedef SQLColumnFormulaVector::iterator SQLColumnFormulaVectorIterator;

class SQLColumnParser
{
public:
	static void init(void);

	SQLColumnParser(void);
	virtual ~SQLColumnParser();
	void setColumnDataGetterFactory
	       (FormulaColumnDataGetterFactory columnDataGetterFactory,
	        void *columnDataGetterFactoryPriv);
	bool add(string& word, string &wordLower);
	bool flush(void);
	const SQLColumnFormulaVector &getColumnFormulaVector(void) const;
	SeparatorCheckerWithCallback *getSeparatorChecker(void);

protected:
	// Type definition
	typedef bool (SQLColumnParser::*FunctionParser)(void);
	typedef map<string, FunctionParser> FunctionParserMap;
	typedef FunctionParserMap::iterator FunctionParserMapIterator;;

	//
	// general sub routines
	//
	void appendFormulaString(const char character);
	void appendFormulaString(string &str);
	FormulaColumn *makeFormulaColumn(string &name);
	void closeCurrentFormulaString(void);
	bool closeCurrentFormula(void);
	bool createdNewElement(FormulaElement *formulaElement);
	FormulaFunction *getFormulaFunctionFromStack(void);
	bool makeFunctionParserIfPendingWordIsFunction(void);
	bool passFunctionArgIfOpen(string &arg);
	bool closeFunctionIfOpen(void);

	//
	// SeparatorChecker callbacks
	//
	static void separatorCbComma
	  (const char separator, SQLColumnParser *columnParser);
	static void separatorCbParenthesisOpen
	  (const char separator, SQLColumnParser *columnParser);
	static void separatorCbParenthesisClose
	  (const char separator, SQLColumnParser *columnParser);

	//
	// functino parsers
	//
	bool funcParserMax(void);

private:
	// Type definition
	struct ParsingContext;

	// Static variables
	static FunctionParserMap m_functionParserMap;

	// General variables
	FormulaColumnDataGetterFactory  m_columnDataGetterFactory;
	void                           *m_columnDataGetterFactoryPriv;
	SQLColumnFormulaVector          m_columnFormulaVector;
	SeparatorCheckerWithCallback    m_separator;
	ParsingContext                 *m_ctx;
};

#endif // SQLColumnParser_h

