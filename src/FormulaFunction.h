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
// FormulaStatisticalFunc
// ---------------------------------------------------------------------------
class FormulaStatisticalFunc : public FormulaFunction {
public:
	FormulaStatisticalFunc(int numArguments = -1);
	virtual void resetStatistics(void) = 0;
};

// ---------------------------------------------------------------------------
// FormulaFuncMax
// ---------------------------------------------------------------------------
class FormulaFuncMax : public FormulaStatisticalFunc {
public:
	FormulaFuncMax(void);
	FormulaFuncMax(FormulaElement *arg);
	virtual ~FormulaFuncMax();
	virtual ItemDataPtr evaluate(void);
	virtual void resetStatistics(void);
};

#endif // FormulaFunction_h
