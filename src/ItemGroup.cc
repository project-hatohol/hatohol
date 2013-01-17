#include "Logger.h"
using namespace mlpl;

#include <stdexcept>
#include "Utils.h"
#include "ItemGroup.h"
#include "ItemGroupTypeManager.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemGroup::ItemGroup(void)
: m_freeze(false),
  m_groupType(NULL)
{
}

void ItemGroup::add(ItemData *data, bool doRef)
{
	writeLock();
	if (m_freeze) {
		writeUnlock();
		throw invalid_argument("Object: freezed");
	}

	ItemDataType itemDataType = data->getItemType();
	if (m_groupType) {
		size_t index = m_itemVector.size();
		ItemDataType expectedType = m_groupType->getType(index);
		if (expectedType != itemDataType) {
			writeUnlock();
			string msg = AMSG("ItemDataType (%d) is not "
			                  "the expected (%d)",
			                  itemDataType, expectedType);
			throw invalid_argument(msg);
		}
		if ((index + 1) == m_groupType->getSize())
			m_freeze = true;
	}

	ItemId itemId = data->getId();
	m_itemMap.insert(pair<ItemId, ItemData *>(itemId, data));
	m_itemVector.push_back(data);
	writeUnlock();
	if (doRef)
		data->ref();
}

ItemData *ItemGroup::getItem(ItemId itemId) const
{
	ItemData *data = NULL;
	readLock();
	ItemDataMapConstIterator it = m_itemMap.find(itemId);
	if (it != m_itemMap.end())
		data = it->second;
	readUnlock();
	return data;
}

ItemData *ItemGroup::getItemAt(size_t index) const
{
	readLock();
	ItemData *data = m_itemVector[index];
	readUnlock();
	return data;
}

size_t ItemGroup::getNumberOfItems(void) const
{
	readLock();
	size_t ret = m_itemVector.size();
	readUnlock();
	return ret;
}

void ItemGroup::freeze(void)
{
	writeLock();
	if (m_freeze) {
		writeUnlock();
		MLPL_WARN("m_freeze: already set.\n");
		return;
	}
	if (m_groupType) {
		writeUnlock();
		string msg = AMSG("m_groupType: NULL\n");
		throw logic_error(msg);
	}

	m_freeze = true;

	ItemGroupTypeManager *typeManager = ItemGroupTypeManager::getInstance();
	m_groupType = typeManager->getItemGroupType(m_itemVector);
	writeUnlock();
}

bool ItemGroup::isFreezed(void) const
{
	readLock();
	bool ret = m_freeze;
	readUnlock();
	return ret;
}

const ItemGroupType *ItemGroup::getItemGroupType(void) const
{
	readLock();
	const ItemGroupType *ret = m_groupType;
	readUnlock();
	return ret;
}

void ItemGroup::setItemGroupType(const ItemGroupType *itemGroupType)
{
	writeLock();
	if (m_groupType) {
		writeUnlock();
		if (m_groupType == itemGroupType) {
			MLPL_WARN("The indentical ItemGroupType is set.\n");
			return;
		}
		string msg = AMSG("m_groupType: alread set.");
		throw logic_error(msg);
	}

	if (!m_itemVector.empty()) {
		string msg = AMSG("m_itemVector is not empty.\n");
		throw invalid_argument(msg);
	}

	m_groupType = itemGroupType;
	writeUnlock();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemGroup::~ItemGroup()
{
	// We don't need to take a lock, because this object is no longer used.
	ItemDataMapIterator it = m_itemMap.begin();
	for (; it != m_itemMap.end(); ++it) {
		ItemData *data = it->second;
		data->unref();
	}
}

