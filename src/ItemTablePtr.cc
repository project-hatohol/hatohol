/* Hatohol
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

#include "ItemTablePtr.h"

template<> VariableItemTablePtr::ItemPtr(void)
: m_data(NULL)
{
	m_data = new ItemTable();
}

template<> ItemTablePtr::ItemPtr(void)
: m_data(NULL)
{
	m_data = new ItemTable();
}

ItemTablePtr
innerJoin(const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1,
          size_t indexLeftJoinColumn, size_t indexRightJoinColumn)
{
	return ItemTablePtr(tablePtr0->innerJoin(tablePtr1,
	                                         indexLeftJoinColumn,
	                                         indexRightJoinColumn), false);
}

ItemTablePtr
leftOuterJoin(const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1)
{
	return ItemTablePtr(tablePtr0->leftOuterJoin(tablePtr1), false);
}

ItemTablePtr
rightOuterJoin(const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1)
{
	return ItemTablePtr(tablePtr0->rightOuterJoin(tablePtr1), false);
}

ItemTablePtr
fullOuterJoin(const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1)
{
	return ItemTablePtr(tablePtr0->fullOuterJoin(tablePtr1), false);
}

ItemTablePtr
crossJoin(const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1)
{
	return ItemTablePtr(tablePtr0->crossJoin(tablePtr1), false);
}
