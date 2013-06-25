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

#include "PrimaryCondition.h"

// ---------------------------------------------------------------------------
// PrimaryCondition
// ---------------------------------------------------------------------------
PrimaryCondition::~PrimaryCondition()
{
}

// ---------------------------------------------------------------------------
// PrimaryConditionColumnsEqual
// ---------------------------------------------------------------------------
PrimaryConditionColumnsEqual::PrimaryConditionColumnsEqual
  (const string &leftTableName, const string &leftColumnName,
   const string &rightTableName, const string &rightColumnName)
: m_leftTableName(leftTableName),
  m_leftColumnName(leftColumnName),
  m_rightTableName(rightTableName),
  m_rightColumnName(rightColumnName)
{
}

const string &PrimaryConditionColumnsEqual::getLeftTableName(void) const
{
	return m_leftTableName;
}

const string &PrimaryConditionColumnsEqual::getLeftColumnName(void) const
{
	return m_leftColumnName;
}

const string &PrimaryConditionColumnsEqual::getRightTableName(void) const
{
	return m_rightTableName;
}

const string &PrimaryConditionColumnsEqual::getRightColumnName(void) const
{
	return m_rightColumnName;
}

// ---------------------------------------------------------------------------
// PrimaryConditionConstants
// ---------------------------------------------------------------------------
PrimaryConditionConstants::PrimaryConditionConstants
  (const string &tableName, const string &columnName)
: m_tableName(tableName),
  m_columnName(columnName)
{
}

const string &PrimaryConditionConstants::getTableName(void) const
{
	return m_tableName;
}

const string &PrimaryConditionConstants::getColumnName(void) const
{
	return m_columnName;
}

void PrimaryConditionConstants::add(ItemDataPtr itemData)
{
	m_itemGroup->add(itemData);
}

const ItemGroupPtr PrimaryConditionConstants::getConstants(void) const
{
	return ItemGroupPtr(m_itemGroup);
}
