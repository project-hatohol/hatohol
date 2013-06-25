/*
 * Copyright (C) 2013 Hatohol Project
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

#ifndef PrimaryCondition_h
#define PrimaryCondition_h

#include <list>
#include <string>
using namespace std;

#include "ItemDataPtr.h"
#include "ItemGroupPtr.h"

// ---------------------------------------------------------------------------
// PrimaryCondition
// ---------------------------------------------------------------------------
class PrimaryCondition {
public:
	virtual ~PrimaryCondition();
};

typedef list<PrimaryCondition *>             PrimaryConditionList;
typedef PrimaryConditionList::iterator       PrimaryConditionListIterator;
typedef PrimaryConditionList::const_iterator PrimaryConditionListConstIterator;

// ---------------------------------------------------------------------------
// PrimaryConditionColumnsEqual
// ---------------------------------------------------------------------------
class PrimaryConditionColumnsEqual : public PrimaryCondition {
public:
	PrimaryConditionColumnsEqual(const string &leftTableName,
	                             const string &leftColumnName,
	                             const string &rightTableName,
	                             const string &rightColumnName);
	const string &getLeftTableName(void) const;
	const string &getLeftColumnName(void) const;
	const string &getRightTableName(void) const;
	const string &getRightColumnName(void) const;

private:
	string m_leftTableName;
	string m_leftColumnName;
	string m_rightTableName;
	string m_rightColumnName;
};

// ---------------------------------------------------------------------------
// PrimaryConditionConstants
// ---------------------------------------------------------------------------
class PrimaryConditionConstants : public PrimaryCondition {
public:
	PrimaryConditionConstants(const string &leftTableName,
	                          const string &leftColumnName);
	const string &getTableName(void) const;
	const string &getColumnName(void) const;
	void add(ItemDataPtr itemData);
	const ItemGroupPtr getConstants(void) const;

private:
	string       m_tableName;
	string       m_columnName;
	VariableItemGroupPtr m_itemGroup;
};

#endif // PrimaryCondition_h


