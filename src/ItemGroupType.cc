#include "ItemGroupType.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemGroupType::ItemGroupType(void)
{
}

ItemGroupType::~ItemGroupType()
{
}

void ItemGroupType::addType(const ItemData *data)
{
	m_typeVector.push_back(data->getItemType());
}

bool ItemGroupType::operator==(const ItemGroupType &itemGroupType) const
{
	return m_typeVector == itemGroupType.m_typeVector;;
}

const vector<ItemDataType> &ItemGroupType::getTypeVector(void) const
{
	return m_typeVector;
}
