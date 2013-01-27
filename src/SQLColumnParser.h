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

class SQLColumnParser
{
public:
	static void init(void);

	SQLColumnParser(void);
	virtual ~SQLColumnParser();
	bool add(string& word, string &wordLower);
	bool flush(void);
	const FormulaElementVector &getFormulaVector(void) const;
	const StringVector         &getFormulaStringVector(void) const;
	const set<string>          &getNameSet(void) const;
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
	FormulaElementVector         m_formulaVector;
	StringVector                 m_formulaStringVector;
	SeparatorCheckerWithCallback m_separator;
	set<string>                  m_nameSet;
	ParsingContext               *m_ctx;
};

#endif // SQLColumnParser_h

