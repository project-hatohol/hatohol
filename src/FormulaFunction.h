/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FormulaFunction_h 
#define FormulaFunction_h

#include "ItemDataPtr.h"
#include "FormulaElement.h"
#include "ItemDataUtils.h"

// ---------------------------------------------------------------------------
// FormulaFunction
// ---------------------------------------------------------------------------
class FormulaFunction : public FormulaElement {
public:
	static const int NUM_ARGUMENTS_VARIABLE = -1;

	FormulaFunction(int m_numArgument = NUM_ARGUMENTS_VARIABLE);
	virtual ~FormulaFunction();
	virtual void resetStatistics(void);
	virtual bool addArgument(FormulaElement *argument);
	virtual bool close(void);

	size_t getNumberOfArguments(void);
	FormulaElement *getArgument(size_t index = 0);
	void pushArgument(FormulaElement *formulaElement);

protected:
	const FormulaElementVector &getArgVector(void) const;

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

private:
	ItemDataPtr m_maxData;
};

// ---------------------------------------------------------------------------
// FormulaFuncCount
// ---------------------------------------------------------------------------
class FormulaFuncCount : public FormulaStatisticalFunc {
public:
	FormulaFuncCount(void);
	virtual ~FormulaFuncCount();
	virtual ItemDataPtr evaluate(void);
	virtual void resetStatistics(void);

	bool isDistinct(void) const;
	void setDistinct(void);

protected:
	static const int NUM_ARGUMENTS_FUNC_COUNT = 1;

private:
	size_t m_count;
	bool   m_isDistinct;
	ItemDataSet m_itemDataSet;
};

// ---------------------------------------------------------------------------
// FormulaFuncSum
// ---------------------------------------------------------------------------
class FormulaFuncSum : public FormulaStatisticalFunc {
public:
	FormulaFuncSum(void);
	virtual ~FormulaFuncSum();
	virtual ItemDataPtr evaluate(void);
	virtual void resetStatistics(void);

protected:
	static const int NUM_ARGUMENTS_FUNC_SUM = 1;

private:
	ItemDataPtr m_dataPtr;
};

#endif // FormulaFunction_h
