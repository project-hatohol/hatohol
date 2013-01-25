#ifndef FormulaFunction_h 
#define FormulaFunction_h

#include "ItemDataPtr.h"
#include "FormulaElement.h"

// ---------------------------------------------------------------------------
// FormulaFunction
// ---------------------------------------------------------------------------
class FormulaFunction : public FormulaElement {
public:
};

// ---------------------------------------------------------------------------
// FormulaFuncMax
// ---------------------------------------------------------------------------
class FormulaFuncMax : public FormulaFunction {
public:
	virtual ItemDataPtr evaluate(const FormulaElement *leftHand,
	                             const FormulaElement *rightHand);
};

#endif // FormulaFunction_h
