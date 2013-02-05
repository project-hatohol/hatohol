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

#include "FormulaOperator.h"

// ---------------------------------------------------------------------------
// Formula Operator
// ---------------------------------------------------------------------------
FormulaOperator::FormulaOperator(void)
{
}

FormulaOperator::~FormulaOperator()
{
}

// ---------------------------------------------------------------------------
// class: FormulaComparatorEqual
// ---------------------------------------------------------------------------
ItemDataPtr FormulaComparatorEqual::evaluate(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemDataPtr();
}

// ---------------------------------------------------------------------------
// FormulaOperatorAnd
// ---------------------------------------------------------------------------
FormulaOperatorAnd::FormulaOperatorAnd(void)
{
}

FormulaOperatorAnd::~FormulaOperatorAnd()
{
}

ItemDataPtr FormulaOperatorAnd::evaluate(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
	return ItemDataPtr();
}
