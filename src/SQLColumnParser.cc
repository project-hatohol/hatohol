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
	m_currFormulaString += word;
	m_pendingWordList.push_back(word);
	return true;
}

bool SQLColumnParser::flush(void)
{
	while (!m_pendingWordList.empty()) {
		string &word = *m_pendingWordList.begin();
		FormulaColumn *formula = new FormulaColumn(word);
		m_formulaVector.push_back(formula);
		m_nameSet.insert(word);
		m_pendingWordList.pop_front();;

		addFormulaString();
	}
	return false;
}

const FormulaElementVector &SQLColumnParser::getFormulaVector(void) const
{
	return m_formulaVector;
}

const StringVector &SQLColumnParser::getFormulaStringVector(void) const
{
	return m_formulaStringVector;
}

SeparatorCheckerWithCallback *SQLColumnParser::getSeparatorChecker(void)
{
	return &m_separator;
}

const set<string> &SQLColumnParser::getNameSet(void) const
{
	return m_nameSet;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void SQLColumnParser::addFormulaString(void)
{
	if (m_currFormulaString.empty())
		return;
	m_formulaStringVector.push_back(m_currFormulaString);
	m_currFormulaString.clear();
}

void SQLColumnParser::separatorCbComma(const char separator,
                                       SQLColumnParser *columnParser)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
	columnParser->addFormulaString();
}

void SQLColumnParser::separatorCbParenthesisOpen(const char separator,
                                                 SQLColumnParser *columnParser)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
	columnParser->m_currFormulaString += separator;
}

void SQLColumnParser::separatorCbParenthesisClose(const char separator,
                                                  SQLColumnParser *columnParser)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
	columnParser->m_currFormulaString += separator;
}

