#ifndef FormulaFunction_h 
#define FormulaFunction_h

#include "ItemDataPtr.h"
#include "FormulaElement.h"

// ---------------------------------------------------------------------------
// FormulaFunction
// ---------------------------------------------------------------------------
class FormulaFunction : public FormulaElement {
public:
	FormulaFunction(int m_numArgument = -1);
	virtual ~FormulaFunction();
	virtual bool addArgument(FormulaElement *argument);
	virtual bool close(void);

	size_t getNumberOfArguments(void);
	FormulaElement *getArgument(size_t index = 0);
	void pushArgument(FormulaElement *formulaElement);

protected:
	const vector<FormulaElement *> &getArgVector(void) const;

private:
	int                      m_numArgument;
	vector<FormulaElement *> m_argVector;
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
};

#endif // FormulaFunction_h
