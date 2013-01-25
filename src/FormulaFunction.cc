#include "FormulaFunction.h"

// ---------------------------------------------------------------------------
// FormulaFuncMax
// ---------------------------------------------------------------------------
FormulaFuncMax::FormulaFuncMax(FormulaElement *arg)
: m_arg(arg)
{
}

FormulaFuncMax::~FormulaFuncMax()
{
	if (m_arg)
		delete m_arg;
}

ItemDataPtr FormulaFuncMax::evaluate(const FormulaElement *leftHand,
                                     const FormulaElement *rightHand)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemDataPtr();
}

bool FormulaFuncMax::addParameter(FormulaElement *parameter)
{
	if (m_arg) {
		MLPL_DBG("m_arg: already set.");
		return false;
	}
	m_arg = parameter;
	return true;
}

bool FormulaFuncMax::close(void)
{
	if (!m_arg) {
		MLPL_DBG("m_arg is NULL.");
		return false;
	}
	return true;
}
