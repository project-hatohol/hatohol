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

#include "DBAgentMySQL.h"

bool DBAgentMySQL::isTableExisting(const string &tableName)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}

bool DBAgentMySQL::isRecordExisting(const string &tableName, const string &condition)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}

void DBAgentMySQL::begin(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::commit(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::rollback(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::createTable(DBAgentTableCreationArg &tableCreationArg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::insert(DBAgentInsertArg &insertArg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::update(DBAgentUpdateArg &updateArg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::select(DBAgentSelectArg &selectArg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::select(DBAgentSelectExArg &selectExArg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::deleteRows(DBAgentDeleteArg &deleteArg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}
