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

#ifndef DBClientUser_h
#define DBClientUser_h

#include "DBClient.h"

struct UserInfo {
	int id;
	string name;
	string password;
};

class DBClientUser : public DBClient {
public:
	static const int   USER_DB_VERSION;
	static const char *DEFAULT_DB_NAME;
	static const char *TABLE_NAME_USERS;
	static void init(void);
	static void reset(void);

	DBClientUser(void);
	virtual ~DBClientUser();

	/**
	 * Add user information to DB.
	 *
	 * @param userInfo
	 * A UserInfo instance that has parameters to be stored.
	 */
	void addUserInfo(UserInfo &userInfo);

protected:
	string sha256(const string &data);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientUser_h
