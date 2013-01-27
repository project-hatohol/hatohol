#include <stdexcept>
#include "SQLColumnParser.h"
#include "FormulaFunction.h"

SQLColumnParser::FunctionParserMap SQLColumnParser::m_functionParserMap;

struct SQLColumnParser::ParsingContext {
	bool                         errorFlag;
	string                       pendingWord;
	string                       pendingWordLower;
	string                       currFormulaString;
	deque<FormulaElement *>      formulaElementStack;

	// constructor and methods
	ParsingContext(void)
	: errorFlag(false)
	{
	}

	void pushPendingWords(string &raw, string &lower) {
		pendingWord = raw;
		pendingWordLower = lower;;
	}

	bool hasPendingWord(void) {
		return !pendingWord.empty();
	}

	void clearPendingWords(void) {
		pendingWord.clear();
		pendingWordLower.clear();
	}
};

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
void SQLColumnParser::init(void)
{
	m_functionParserMap["max"] = &SQLColumnParser::funcParserMax;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLColumnParser::SQLColumnParser(void)
: m_columnDataGetterFactory(NULL),
  m_columnDataGetterFactoryPriv(NULL),
  m_separator(" ,()")
{
	m_ctx = new ParsingContext;

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

	if (m_ctx)
		delete m_ctx;
}

void SQLColumnParser::setColumnDataGetterFactory
       (FormulaColumnDataGetterFactory columnDataGetterFactory,
        void *columnDataGetterFactoryPriv)
{
       m_columnDataGetterFactory = columnDataGetterFactory;
       m_columnDataGetterFactoryPriv = columnDataGetterFactoryPriv;
}

bool SQLColumnParser::add(string &word, string &wordLower)
{
	if (m_ctx->errorFlag)
		return false;

	if (m_ctx->hasPendingWord()) {
		MLPL_DBG("hasPendingWord(): true.");
		return false;
	}

	if (passFunctionArgIfOpen(word))
		return true;

	m_ctx->pushPendingWords(word, wordLower);
	return true;
}

bool SQLColumnParser::flush(void)
{
	if (m_ctx->errorFlag)
		return false;

	if (!m_ctx->hasPendingWord()) {
		closeCurrentFormulaString();
		return closeCurrentFormula();
	}

	appendFormulaString(m_ctx->pendingWord);
	FormulaColumn *formulaColumn = makeFormulaColumn(m_ctx->pendingWord);
	m_ctx->clearPendingWords();
	closeCurrentFormulaString();
	if (!createdNewElement(formulaColumn))
		return false;
	return closeCurrentFormula();
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
//
// general sub routines
//
void SQLColumnParser::appendFormulaString(const char character)
{
	m_ctx->currFormulaString += character;
}

void SQLColumnParser::appendFormulaString(string &str)
{
	m_ctx->currFormulaString += str;
}

FormulaColumn *SQLColumnParser::makeFormulaColumn(string &name)
{
	if (!m_columnDataGetterFactory) {
		string msg;
		TRMSG(msg, "m_columnDataGetterFactory: NULL.");
		throw logic_error(msg);
	}
	FormulaColumnDataGetter *dataGetter =
	  (*m_columnDataGetterFactory)(m_columnDataGetterFactoryPriv);
	FormulaColumn *formulaColumn = new FormulaColumn(name, dataGetter);
	m_nameSet.insert(name);
	return formulaColumn;
}

void SQLColumnParser::closeCurrentFormulaString(void)
{
	if (m_ctx->currFormulaString.empty())
		return;
	m_formulaStringVector.push_back(m_ctx->currFormulaString);
	m_ctx->currFormulaString.clear();
}

bool SQLColumnParser::closeCurrentFormula(void)
{
	if (m_ctx->formulaElementStack.empty())
		return true;

	FormulaFunction *formulaFunc = getFormulaFunctionFromStack();
	if (formulaFunc) {
		MLPL_DBG("The current function is not closed.");
		return false;;
	}

	m_ctx->formulaElementStack.pop_back();
	if (!m_ctx->formulaElementStack.empty()) {
		MLPL_DBG("formulaElementStat is not empty (%zd).\n",
		         m_ctx->formulaElementStack.size());
		return false;
	}
	return true;
}

bool SQLColumnParser::createdNewElement(FormulaElement *formulaElement)
{
	if (m_ctx->formulaElementStack.empty())
		m_formulaVector.push_back(formulaElement);
	m_ctx->formulaElementStack.push_back(formulaElement);
	return true;
}

bool SQLColumnParser::makeFunctionParserIfPendingWordIsFunction(void)
{
	if (!m_ctx->hasPendingWord())
		return false;

	FunctionParserMapIterator it;
	it = m_functionParserMap.find(m_ctx->pendingWordLower);
	if (it == m_functionParserMap.end())
		return false;

	FunctionParser func = it->second;
	if (!(this->*func)())
		m_ctx->errorFlag = false;

	appendFormulaString(m_ctx->pendingWord);
	m_ctx->clearPendingWords();

	return true;
}

FormulaFunction *SQLColumnParser::getFormulaFunctionFromStack(void)
{
	if (m_ctx->formulaElementStack.empty())
		return NULL;
	FormulaElement *elem = m_ctx->formulaElementStack.back();
	return dynamic_cast<FormulaFunction *>(elem);
}

bool SQLColumnParser::passFunctionArgIfOpen(string &word)
{
	FormulaFunction *formulaFunc = getFormulaFunctionFromStack();
	if (!formulaFunc)
		return false;
	FormulaColumn *formulaColumn = makeFormulaColumn(word);
	if (!formulaFunc->addArgument(formulaColumn))
		return false;
	appendFormulaString(word);
	return true;
}

bool SQLColumnParser::closeFunctionIfOpen(void)
{
	FormulaFunction *formulaFunc = getFormulaFunctionFromStack();
	if (!formulaFunc)
		return false;

	if (!formulaFunc->close())
		m_ctx->errorFlag = true;
	m_ctx->formulaElementStack.pop_back();
	return true;
}

//
// SeparatorChecker callbacks
//
void SQLColumnParser::separatorCbComma(const char separator,
                                       SQLColumnParser *columnParser)
{
	columnParser->flush();
}

void SQLColumnParser::separatorCbParenthesisOpen(const char separator,
                                                 SQLColumnParser *columnParser)
{
	bool isFunc;
	isFunc = columnParser->makeFunctionParserIfPendingWordIsFunction();
	columnParser->appendFormulaString(separator);
	if (isFunc)
		return;

	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
}

void SQLColumnParser::separatorCbParenthesisClose(const char separator,
                                                  SQLColumnParser *columnParser)
{
	columnParser->appendFormulaString(separator);

	if (columnParser->closeFunctionIfOpen())
		return;

	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
}

//
// functino parsers
//
bool SQLColumnParser::funcParserMax(void)
{
	FormulaFuncMax *funcMax = new FormulaFuncMax();
	return createdNewElement(funcMax);
}

