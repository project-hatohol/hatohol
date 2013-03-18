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

#ifndef ColumnComparisonPicker_h
#define ColumnComparisonPicker_h

#include <list>
#include <string>
using namespace std;

#include "FormulaElement.h"
#include "FormulaOperator.h"

class ColumnComparisonPicker {
public:
	virtual ~ColumnComparisonPicker();
	void pickupPrimary(FormulaElement *formula);
	const ColumnComparisonInfoList &getColumnComparisonInfoList(void) const;
protected:
	bool picupPrimaryRecursive
	       (ColumnComparisonInfoList &columnCompList,
	        FormulaElement *formula);
	bool pickupComparatorEqual
	       (ColumnComparisonInfoList &columnCompList,
	        FormulaComparatorEqual *compEq);
	bool pickupOperatorAnd
	       (ColumnComparisonInfoList &columnCompList,
	        FormulaOperatorAnd *operatorAnd);
private:
	ColumnComparisonInfoList m_columnCompList;
};

#endif // FormulaVariableComaprisonPicker_h
