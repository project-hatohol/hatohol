#include <stdexcept>

#include "FormulaFunction.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
// FormulaFunction
// ---------------------------------------------------------------------------
FormulaFunction::FormulaFunction(int numArgument)
: m_numArgument(numArgument)
{
}

FormulaFunction::~FormulaFunction()
{
	for (size_t i = 0; i < getNumberOfArguments(); i++)
		delete m_argVector[i];
}

size_t FormulaFunction::getNumberOfArguments(void)
{
	return m_argVector.size();
}

FormulaElement *FormulaFunction::getArgument(size_t index)
{
	if (index >= getNumberOfArguments()) {
		string msg;
		TRMSG(msg, "indxe (%zd) >= array sizei (%zd)",
		      index, getNumberOfArguments());
		throw invalid_argument(msg);
	}
	return m_argVector[index];
}

void FormulaFunction::pushArgument(FormulaElement *formulaElement)
{
	m_argVector.push_back(formulaElement);
}

//
// Virtual methods
//
bool FormulaFunction::addArgument(FormulaElement *argument)
{
	if (m_numArgument >= 0) {
		if (getNumberOfArguments() >= (size_t)m_numArgument) {
			MLPL_DBG("Too many arguemnts.");
			return false;
		}
		pushArgument(argument);
	}
	return true;
}

bool FormulaFunction::close(void)
{
	if (m_numArgument >= 0) {
		if (getNumberOfArguments() != (size_t)m_numArgument) {
			MLPL_DBG("Number of argument is short: %zd / %zd.\n",
			         getNumberOfArguments(), m_numArgument);
			return false;
		}
	}
	return true;
}

//
// Prtected methods
//
const FormulaElementVector &FormulaFunction::getArgVector(void) const
{
	return m_argVector;
}

// ---------------------------------------------------------------------------
// FormulaStatisticalFunc
// ---------------------------------------------------------------------------
FormulaStatisticalFunc::FormulaStatisticalFunc(int numArguments)
: FormulaFunction(numArguments)
{
}

// ---------------------------------------------------------------------------
// FormulaFuncMax
// ---------------------------------------------------------------------------
FormulaFuncMax::FormulaFuncMax(void)
: FormulaStatisticalFunc(NUM_ARGUMENTS_FUNC_MAX)
{
}

FormulaFuncMax::FormulaFuncMax(FormulaElement *arg)
{
	pushArgument(arg);
}

FormulaFuncMax::~FormulaFuncMax()
{
}

ItemDataPtr FormulaFuncMax::evaluate(void)
{
	// TODO: get value of the child element and compare the current max.
	return m_maxData;
}

void FormulaFuncMax::resetStatistics(void)
{
	m_maxData = ItemDataPtr();
}
