/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PrimaryConditionPicker_h
#define PrimaryConditionPicker_h

#include <list>
#include <string>
using namespace std;

#include "FormulaElement.h"
#include "FormulaOperator.h"

class PrimaryConditionPicker {
public:
	virtual ~PrimaryConditionPicker();
	void pickupPrimary(FormulaElement *formula);
	const PrimaryConditionList &getPrimaryConditionList(void) const;
protected:
	bool picupPrimaryRecursive
	       (PrimaryConditionList &primaryConditionList,
	        FormulaElement *formula);
	bool pickupComparatorEqual
	       (PrimaryConditionList &primaryConditionList,
	        FormulaComparatorEqual *compEq);
	bool pickupOperatorAnd
	       (PrimaryConditionList &primaryConditionList,
	        FormulaOperatorAnd *operatorAnd);
	bool pickupFormulaIn
	       (PrimaryConditionList &primaryConditionList,
	        FormulaIn *formulaIn);

private:
	PrimaryConditionList m_primaryConditionList;
};

#endif // FormulaVariableComaprisonPicker_h
