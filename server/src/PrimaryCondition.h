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

#ifndef PrimaryCondition_h
#define PrimaryCondition_h

#include <list>
#include <string>
#include "ItemDataPtr.h"
#include "ItemGroupPtr.h"

// ---------------------------------------------------------------------------
// PrimaryCondition
// ---------------------------------------------------------------------------
class PrimaryCondition {
public:
	virtual ~PrimaryCondition();
};

typedef std::list<PrimaryCondition *>        PrimaryConditionList;
typedef PrimaryConditionList::iterator       PrimaryConditionListIterator;
typedef PrimaryConditionList::const_iterator PrimaryConditionListConstIterator;

// ---------------------------------------------------------------------------
// PrimaryConditionColumnsEqual
// ---------------------------------------------------------------------------
class PrimaryConditionColumnsEqual : public PrimaryCondition {
public:
	PrimaryConditionColumnsEqual(const std::string &leftTableName,
	                             const std::string &leftColumnName,
	                             const std::string &rightTableName,
	                             const std::string &rightColumnName);
	const std::string &getLeftTableName(void) const;
	const std::string &getLeftColumnName(void) const;
	const std::string &getRightTableName(void) const;
	const std::string &getRightColumnName(void) const;

private:
	std::string m_leftTableName;
	std::string m_leftColumnName;
	std::string m_rightTableName;
	std::string m_rightColumnName;
};

// ---------------------------------------------------------------------------
// PrimaryConditionConstants
// ---------------------------------------------------------------------------
class PrimaryConditionConstants : public PrimaryCondition {
public:
	PrimaryConditionConstants(const std::string &leftTableName,
	                          const std::string &leftColumnName);
	const std::string &getTableName(void) const;
	const std::string &getColumnName(void) const;
	void add(ItemDataPtr itemData);
	const ItemGroupPtr getConstants(void) const;

private:
	std::string          m_tableName;
	std::string          m_columnName;
	VariableItemGroupPtr m_itemGroup;
};

#endif // PrimaryCondition_h


