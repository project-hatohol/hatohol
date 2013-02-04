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

#include <stdexcept>
#include "ItemGroupType.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
// Public methods (ItemGroupTypeSetComp)
// ---------------------------------------------------------------------------
bool ItemGroupTypeSetComp::operator()(const ItemGroupType *groupType0,
                                      const ItemGroupType *groupType1) const
{
	return *groupType0 < *groupType1;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemGroupType::ItemGroupType(const ItemDataVector &itemDataVector)
{
	for (size_t i = 0; i < itemDataVector.size(); i++) {
		const ItemData *data = itemDataVector[i];
		m_typeVector.push_back(data->getItemType());
	}
}

ItemGroupType::~ItemGroupType()
{
}

const size_t ItemGroupType::getSize() const
{
	return m_typeVector.size();
}

ItemDataType ItemGroupType::getType(size_t index) const
{
	ItemDataType ret;
	if (index >= m_typeVector.size()) {
		string msg;
		TRMSG(msg, "index (%zd) >= Vector size (%zd).",
		      index, m_typeVector.size());
		throw out_of_range(msg);
	}
	ret = m_typeVector[index];
	return ret;
}

bool ItemGroupType::operator<(const ItemGroupType &itemGroupType) const
{
	size_t size0 = getSize();
	size_t size1 = itemGroupType.getSize();
	if (size0 != size1)
		return size0 < size1;
	for (size_t i = 0; i < size0; i++) {
		if (m_typeVector[i] < itemGroupType.m_typeVector[i])
			return true;
	}
	return false;
}

bool ItemGroupType::operator==(const ItemGroupType &itemGroupType) const
{
	if (this == &itemGroupType)
		return true;

	size_t size0 = getSize();
	size_t size1 = itemGroupType.getSize();
	if (size0 != size1)
		return false;
	for (size_t i = 0; i < size0; i++) {
		if (m_typeVector[i] != itemGroupType.m_typeVector[i])
			return false;
	}
	return true;
}

bool ItemGroupType::operator!=(const ItemGroupType &itemGroupType) const
{
	return (*this == itemGroupType) ? false : true;
}

