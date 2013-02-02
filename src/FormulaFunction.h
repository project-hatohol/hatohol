#ifndef FormulaFunction_h 
#define FormulaFunction_h

#include "ItemDataPtr.h"
#include "FormulaElement.h"

// ---------------------------------------------------------------------------
// FormulaFunction
// ---------------------------------------------------------------------------
class FormulaFunction : public FormulaElement {
public:
	static const int NUM_ARGUMENTS_VARIABLE = -1;

	FormulaFunction(int m_numArgument = NUM_ARGUMENTS_VARIABLE);
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
	FormulaStatisticalFunc(int numArguments = NUM_ARGUMENTS_VARIABLE);
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

protected:
	static const int NUM_ARGUMENTS_FUNC_MAX = 1;

protected:
	ItemDataPtr m_maxData;
};

#endif // FormulaFunction_h
