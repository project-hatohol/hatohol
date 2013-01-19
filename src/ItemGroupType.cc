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

