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

#include "ColumnComparisonPicker.h"
#include "SQLUtils.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ColumnComparisonPicker::~ColumnComparisonPicker()
{
	ColumnComparisonInfoListIterator it = m_columnCompList.begin();
	for (; it != m_columnCompList.end(); ++it)
		delete *it;
}

void ColumnComparisonPicker::pickupPrimary
  (FormulaElement *formula)
{
	picupPrimaryRecursive(m_columnCompList, formula);
}

const ColumnComparisonInfoList &
ColumnComparisonPicker::getColumnComparisonInfoList(void) const
{
	return m_columnCompList;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool ColumnComparisonPicker::picupPrimaryRecursive
  (ColumnComparisonInfoList &columnCompList, FormulaElement *formula)
{
	FormulaComparatorEqual *compEq =
	   dynamic_cast<FormulaComparatorEqual *>(formula);
	if (compEq)
		return pickupComparatorEqual(columnCompList, compEq);

	FormulaOperatorAnd *operatorAnd =
	   dynamic_cast<FormulaOperatorAnd *>(formula);
	if (operatorAnd)
		return pickupOperatorAnd(columnCompList, operatorAnd);

	FormulaIn *formulaIn = dynamic_cast<FormulaIn *>(formula);
	if (formulaIn)
		return pickupFormulaIn(columnCompList, formulaIn);

	return false;
}

bool ColumnComparisonPicker::pickupComparatorEqual
  (ColumnComparisonInfoList &columnCompList, FormulaComparatorEqual *compEq)
{
	FormulaVariable *leftVariable =
	   dynamic_cast<FormulaVariable *>(compEq->getLeftHand());
	if (!leftVariable)
		return false;

	FormulaVariable *rightVariable =
	   dynamic_cast<FormulaVariable *>(compEq->getRightHand());
	if (!rightVariable)
		return false;

	ColumnComparisonInfo *columnCompInfo = new ColumnComparisonInfo();
	SQLUtils::decomposeTableAndColumn(leftVariable->getName(),
	                                  columnCompInfo->leftTableName,
	                                  columnCompInfo->leftColumnName);
	SQLUtils::decomposeTableAndColumn(rightVariable->getName(),
	                                  columnCompInfo->rightTableName,
	                                  columnCompInfo->rightColumnName);
	columnCompList.push_back(columnCompInfo);
	return true;
}

bool ColumnComparisonPicker::pickupOperatorAnd
  (ColumnComparisonInfoList &columnCompList, FormulaOperatorAnd *operatorAnd)
{
	FormulaElement *leftElem = operatorAnd->getLeftHand();
	picupPrimaryRecursive(columnCompList, leftElem);

	FormulaElement *rightElem = operatorAnd->getRightHand();
	picupPrimaryRecursive(columnCompList, rightElem);
	return true;
}

bool ColumnComparisonPicker::pickupFormulaIn
  (ColumnComparisonInfoList &columnCompList, FormulaIn *formulaIn)
{
	FormulaVariable *leftVariable =
	   dynamic_cast<FormulaVariable *>(formulaIn->getLeftHand());
	if (!leftVariable)
		return false;

	// TODO: make a ColumnComparisonInfo instance and push it to the list.

	return true;
}
