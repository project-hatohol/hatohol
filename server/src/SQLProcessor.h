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

#ifndef SQLProcessor_h
#define SQLProcessor_h

#include <string>
#include <vector>
#include <map>
#include <list>
using namespace std;

#include "StringUtils.h"
#include "ParsableString.h"
using namespace mlpl;

#include <glib.h>
#include "ItemGroupPtr.h"
#include "ItemTablePtr.h"
#include "FormulaElement.h"
#include "SQLColumnParser.h"
#include "SQLFromParser.h"
#include "SQLWhereParser.h"
#include "SQLProcessorTypes.h"
#include "SQLProcessorSelect.h"
#include "SQLProcessorInsert.h"
#include "SQLProcessorUpdate.h"

class SQLProcessor
{
private:
	struct PrivateContext;

public:
	virtual bool select(SQLSelectInfo &selectInfo);
	virtual bool insert(SQLInsertInfo &insertInfo);
	virtual bool update(SQLUpdateInfo &updateInfo);
	virtual const char *getDBName(void) = 0;

protected:
	/**
	 * @tableNameStaticInfoMap A map whose key is a table name and
	 *                         the value is a SQLTableStaticInfo instance.
	 *                         The body of it is typicall allocated in
	 *                         the sub class.
	 */
	SQLProcessor(const string &dbName,
	             TableNameStaticInfoMap &tableNameStaticInfoMap);
	virtual ~SQLProcessor();

private:
	SQLProcessorSelect           m_processorSelect;
	SQLProcessorInsert           m_processorInsert;
	SQLProcessorUpdate           m_processorUpdate;
};

#endif // SQLProcessor_h
