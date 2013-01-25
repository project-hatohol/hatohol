#ifndef FormulaOperator_h 
#define FormulaOperator_h

#include "ItemDataPtr.h"

class FormulaElement;

// ---------------------------------------------------------------------------
// FormulaOperator
// ---------------------------------------------------------------------------
class FormulaOperator {
public:
	FormulaOperator(void);
	virtual ~FormulaOperator();
	virtual ItemDataPtr evaluate(const FormulaElement *leftHand,
	                             const FormulaElement *rightHand) = 0;
};

#endif // FormulaOperator_h
