#include "ItemTablePtr.h"

template<>
ItemPtr<ItemTable>::ItemPtr(void)
: m_data(NULL)
{
	m_data = new ItemTable();
}

ItemTablePtr
innerJoin(const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1)
{
	return ItemTablePtr(tablePtr0->innerJoin(tablePtr1), false);
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
