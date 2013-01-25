#include "SQLColumnParser.h"

SQLColumnParser::FunctionParserMap SQLColumnParser::m_functionParserMap;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
void SQLColumnParser::init(void)
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLColumnParser::SQLColumnParser(void)
: m_separator(" ,()")
{
	m_separator.setCallbackTempl<SQLColumnParser>
	  (',', separatorCbComma, this);
	m_separator.setCallbackTempl<SQLColumnParser>
	  ('(', separatorCbParenthesisOpen, this);
	m_separator.setCallbackTempl<SQLColumnParser>
	  (')', separatorCbParenthesisClose, this);
}

SQLColumnParser::~SQLColumnParser()
{
	FormulaElementVectorIterator formulaIt = m_formulaVector.begin();
	for(; formulaIt != m_formulaVector.end(); ++formulaIt)
		delete *formulaIt;
}

bool SQLColumnParser::add(string &word, string &wordLower)
{
	FunctionParserMapIterator it = m_functionParserMap.find(wordLower);
	if (it != m_functionParserMap.end()) {
		FunctionParser funcParser = it->second;
		return (this->*funcParser)();
	}
	m_pendingWordList.push_back(word);
	return false;
}

bool SQLColumnParser::flush(void)
{
	while (!m_pendingWordList.empty()) {
		string &word = *m_pendingWordList.begin();
		FormulaColumn *formula = new FormulaColumn(word);
		m_formulaVector.push_back(formula);
		m_pendingWordList.pop_front();;
	}
	return false;
}

const FormulaElementVector &SQLColumnParser::getFormulaVector(void) const
{
	return m_formulaVector;
}

SeparatorCheckerWithCallback *SQLColumnParser::getSeparatorChecker(void)
{
	return &m_separator;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void SQLColumnParser::separatorCbComma(const char separator,
                                       SQLColumnParser *columnParser)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
}

void SQLColumnParser::separatorCbParenthesisOpen(const char separator,
                                                 SQLColumnParser *columnParser)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
}

void SQLColumnParser::separatorCbParenthesisClose(const char separator,
                                                  SQLColumnParser *columnParser)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
}

