#ifndef FormulaFunction_h 
#define FormulaFunction_h

#include "ItemDataPtr.h"
#include "FormulaElement.h"

// ---------------------------------------------------------------------------
// FormulaFunction
// ---------------------------------------------------------------------------
class FormulaFunction : public FormulaElement {
public:
	virtual bool addParameter(FormulaElement *parameter) = 0;
	virtual bool close(void) = 0;
};

// ---------------------------------------------------------------------------
// FormulaFuncMax
// ---------------------------------------------------------------------------
class FormulaFuncMax : public FormulaFunction {
public:
	FormulaFuncMax(void);
	FormulaFuncMax(FormulaElement *arg);
	virtual ~FormulaFuncMax();
	virtual ItemDataPtr evaluate(const FormulaElement *leftHand,
	                             const FormulaElement *rightHand);
	virtual bool addParameter(FormulaElement *parameter);
	virtual bool close(void);
public:
	FormulaElement *m_arg;
};

#endif // FormulaFunction_h
