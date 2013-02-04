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

#ifndef FormulaElement_h 
#define FormulaElement_h

#include <vector>
using namespace std;

#include "ItemDataPtr.h"
#include "FormulaOperator.h"

// ---------------------------------------------------------------------------
// class: FormulaElement
// ---------------------------------------------------------------------------
class FormulaElement {
public:
	FormulaElement(void);
	virtual ~FormulaElement();
	void setLeftHand(FormulaElement *elem);
	void setRightHand(FormulaElement *elem);
	void setOperator(FormulaOperator *op);

	FormulaElement *getLeftHand(void) const;
	FormulaElement *getRightHand(void) const;

	virtual ItemDataPtr evaluate(void);

private:
	FormulaElement  *m_leftHand;
	FormulaElement  *m_rightHand;
	FormulaOperator *m_operator;
	FormulaElement  *m_parent;
};

typedef vector<FormulaElement *>       FormulaElementVector;
typedef FormulaElementVector::iterator FormulaElementVectorIterator;

// ---------------------------------------------------------------------------
// class: FormulaCompareEqual
// ---------------------------------------------------------------------------
class FormulaCompareEqual : public FormulaElement {
public:
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// class: FormulaValue
// ---------------------------------------------------------------------------
class FormulaValue : public FormulaElement {
public:
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// class: FormulaVariable
// ---------------------------------------------------------------------------
class FormulaVariable;
class FormulaVariableDataGetter {
public:
	virtual ~FormulaVariableDataGetter() {}
	virtual ItemDataPtr getData(void) = 0;
};

typedef FormulaVariableDataGetter *
(*FormulaVariableDataGetterFactory)(string &name, void *priv);

class FormulaVariable : public FormulaElement {
public:
	FormulaVariable(string &name,
	              FormulaVariableDataGetter *variableDataGetter);
	virtual ~FormulaVariable();
	virtual ItemDataPtr evaluate(void);

	const string &getName(void) const;
	FormulaVariableDataGetter *getFormulaVariableGetter(void) const;

private:
	string                     m_name;
	FormulaVariableDataGetter *m_variableGetter;
};

#endif // FormulaElement_h
