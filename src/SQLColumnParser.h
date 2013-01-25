#ifndef SQLColumnParser_h
#define SQLColumnParser_h

#include <string>
#include <list>
using namespace std;

#include "ParsableString.h"
using namespace mlpl;

#include "FormulaElement.h"

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

	void addFormulaString(void);

	//
	// SeparatorChecker callbacks
	//
	static void separatorCbComma
	  (const char separator, SQLColumnParser *columnParser);
	static void separatorCbParenthesisOpen
	  (const char separator, SQLColumnParser *columnParser);
	static void separatorCbParenthesisClose
	  (const char separator, SQLColumnParser *columnParser);

private:
	// Type definition
	typedef bool (SQLColumnParser::*FunctionParser)(void);
	typedef map<string, FunctionParser> FunctionParserMap;
	typedef FunctionParserMap::iterator FunctionParserMapIterator;;

	// Static variables
	static FunctionParserMap m_functionParserMap;

	// Non-static variables
	FormulaElementVector         m_formulaVector;
	StringVector                 m_formulaStringVector;
	SeparatorCheckerWithCallback m_separator;
	set<string>                  m_nameSet;
	list<string>                 m_pendingWordList;

	string                       m_currFormulaString;
};

#endif // SQLColumnParser_h

