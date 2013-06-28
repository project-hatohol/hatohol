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

#ifndef ItemGroupType_h
#define ItemGroupType_h

#include <set>
using namespace std;

#include "ItemData.h"

/**
 * Methods except constructor in this class are 'const'. So they are
 * multi-thread safe.
 */
class ItemGroupType {
public:
	/**
	 * itemDataVector must not be changed by other thread
	 * while this function is being called.
	 */
	ItemGroupType(const ItemDataVector &itemDataVector);

	virtual ~ItemGroupType();
	const size_t getSize() const;
	ItemDataType getType(size_t index) const;
	bool operator<(const ItemGroupType &itemGroupType) const;
	bool operator==(const ItemGroupType &itemGroupType) const;
	bool operator!=(const ItemGroupType &itemGroupType) const;

private:
	vector<ItemDataType> m_typeVector;
};

struct ItemGroupTypeSetComp {
	bool operator()(const ItemGroupType *t0, const ItemGroupType *t1) const;
};

typedef set<ItemGroupType *, ItemGroupTypeSetComp> ItemGroupTypeSet;
typedef ItemGroupTypeSet::iterator       ItemGroupTypeSetIterator;
typedef ItemGroupTypeSet::const_iterator ItemGroupTypeSetConstIterator;

#endif // ItemGroupType_h

