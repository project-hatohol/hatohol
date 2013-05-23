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

struct DBAgentMySQL::PrivateContext {
	MYSQL mysql;
	bool  connected;

	PrivateContext(void)
	: connected(false)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBAgentMySQL::DBAgentMySQL(const char *db)
: m_ctx(NULL)
{
	const char *host = NULL; // localhost is used.
	const char *user = NULL; // current user name is used.
	const char *passwd = NULL; // passwd is not checked.
	unsigned int port = 0; // default port is used.
	const char *unixSocket = NULL;
	unsigned long clientFlag = 0;
	m_ctx = new PrivateContext();
	mysql_init(&m_ctx->mysql);
	MYSQL *result = mysql_real_connect(&m_ctx->mysql, host, user, passwd,
	                                   db, port, unixSocket, clientFlag);
	if (!result) {
		THROW_ASURA_EXCEPTION("Failed to connect to MySQL: %s: %s\n",
		                      db, mysql_error(&m_ctx->mysql));
	}
	m_ctx->connected = true;
}

DBAgentMySQL::~DBAgentMySQL()
{
	if (m_ctx) {
		mysql_close(&m_ctx->mysql);
		delete m_ctx;
	}
}

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
