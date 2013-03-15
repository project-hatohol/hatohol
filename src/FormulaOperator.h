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

#ifndef FormulaOperator_h 
#define FormulaOperator_h

#include "FormulaElement.h"
#include "ItemDataPtr.h"
#include "ItemGroupPtr.h"
#include "SQLProcessorTypes.h"
#include "SQLProcessorSelect.h"

// ---------------------------------------------------------------------------
// class: FormulaParenthesis
// ---------------------------------------------------------------------------
class FormulaParenthesis : public FormulaElement {
public:
	FormulaParenthesis(void);
	virtual ~FormulaParenthesis();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// class: FormulaComparatorEqual
// ---------------------------------------------------------------------------
class FormulaComparatorEqual : public FormulaElement {
public:
	FormulaComparatorEqual(void);
	virtual ~FormulaComparatorEqual();
	virtual FormulaOptimizationResult optimize(void);
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// class: FormulaComparatorNotEqual
// ---------------------------------------------------------------------------
class FormulaComparatorNotEqual : public FormulaElement {
public:
	FormulaComparatorNotEqual(void);
	virtual ~FormulaComparatorNotEqual();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// class: FormulaGreaterThan
// ---------------------------------------------------------------------------
class FormulaGreaterThan : public FormulaElement {
public:
	FormulaGreaterThan(void);
	virtual ~FormulaGreaterThan();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// class: FormulaGreaterOrEqual
// ---------------------------------------------------------------------------
class FormulaGreaterOrEqual : public FormulaElement {
public:
	FormulaGreaterOrEqual(void);
	virtual ~FormulaGreaterOrEqual();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// class: FormulaOperatorPlus
// ---------------------------------------------------------------------------
class FormulaOperatorPlus : public FormulaElement {
public:
	FormulaOperatorPlus(void);
	virtual ~FormulaOperatorPlus();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// class: FormulaOperatorDiv
// ---------------------------------------------------------------------------
class FormulaOperatorDiv : public FormulaElement {
public:
	FormulaOperatorDiv(void);
	virtual ~FormulaOperatorDiv();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// FormulaOperatorNot
// ---------------------------------------------------------------------------
class FormulaOperatorNot : public FormulaElement {
public:
	FormulaOperatorNot(void);
	virtual ~FormulaOperatorNot();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// FormulaOperatorAnd
// ---------------------------------------------------------------------------
class FormulaOperatorAnd : public FormulaElement {
public:
	FormulaOperatorAnd(void);
	virtual ~FormulaOperatorAnd();
	virtual FormulaOptimizationResult optimize(void);
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// FormulaOperatorOr
// ---------------------------------------------------------------------------
class FormulaOperatorOr : public FormulaElement {
public:
	FormulaOperatorOr(void);
	virtual ~FormulaOperatorOr();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// FormulaBetween
// ---------------------------------------------------------------------------
class FormulaBetween : public FormulaElement {
public:
	FormulaBetween(ItemDataPtr v0, ItemDataPtr v1);
	virtual ~FormulaBetween();
	virtual ItemDataPtr evaluate(void);
	ItemDataPtr getV0(void) const;
	ItemDataPtr getV1(void) const;

private:
	ItemDataPtr      m_v0;
	ItemDataPtr      m_v1;
};

// ---------------------------------------------------------------------------
// FormulaIn
// ---------------------------------------------------------------------------
class FormulaIn : public FormulaElement {
public:
	FormulaIn(ItemGroupPtr values);
	virtual ~FormulaIn();
	virtual ItemDataPtr evaluate(void);
	const ItemGroupPtr getValues(void) const;

private:
	ItemGroupPtr m_values;
};

// ---------------------------------------------------------------------------
// FormulaExists
// ---------------------------------------------------------------------------
class FormulaExists : public FormulaElement {
public:
	FormulaExists(const string &statement,
	              SQLProcessorSelectFactory &selectFactory);
	virtual ~FormulaExists();
	virtual ItemDataPtr evaluate(void);
	const string &getStatement(void) const;

private:
	SQLProcessorSelectFactory &m_processorSelectFactory;
	string                     m_statement;
	SQLProcessorSelect        *m_processorSelect;
	SQLSelectInfo             *m_selectInfo;
};

// ---------------------------------------------------------------------------
// FormulaIsNull
// ---------------------------------------------------------------------------
class FormulaIsNull : public FormulaElement {
public:
	FormulaIsNull(void);
	virtual ~FormulaIsNull();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// FormulaIsNotNull
// ---------------------------------------------------------------------------
class FormulaIsNotNull : public FormulaElement {
public:
	FormulaIsNotNull(void);
	virtual ~FormulaIsNotNull();
	virtual ItemDataPtr evaluate(void);
};

#endif // FormulaOperator_h
