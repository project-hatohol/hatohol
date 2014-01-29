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

#ifndef OperationPrivilege_h
#define OperationPrivilege_h

#include <string>
#include "Params.h"

enum OperationPrivilegeType
{
	// User
	// Without any flags, user info of caller itself can be got.
	OPPRVLG_CREATE_USER,
	OPPRVLG_UPDATE_USER,   // can update all users
	OPPRVLG_DELETE_USER,   // can delete all users 
	OPPRVLG_GET_ALL_USER,  // can get all users. 

	// Server
	// Without any flags, servers that are allowed by AccessInfoList
	// can be got.
	OPPRVLG_CREATE_SERVER, 
	OPPRVLG_UPDATE_SERVER,        // can only update accessible servers
	OPPRVLG_UPDATE_ALL_SERVER,    // can update all servers
	OPPRVLG_DELETE_SERVER,        // can only delete accessible servers
	OPPRVLG_DELETE_ALL_SERVER,    // can delete all servers
	OPPRVLG_GET_ALL_SERVER,       // can get all servers

	// Action
	// Without any flags, own actions can be got.
	OPPRVLG_CREATE_ACTION,         // create own action
	OPPRVLG_UPDATE_ACTION,         // can only update own action
	OPPRVLG_UPDATE_ALL_ACTION,     // can updat all actions
	OPPRVLG_DELETE_ACTION,         // can only delete own actioin
	OPPRVLG_DELETE_ALL_ACTION,     // can delte all actions
	OPPRVLG_GET_ALL_ACTION,        // can get all actions

	// User role
	// All users can get all user roles.
	// Without any flags, a user can't edit user roles
	OPPRVLG_CREATE_USER_ROLE,
	OPPRVLG_UPDATE_ALL_USER_ROLE,
	OPPRVLG_DELETE_ALL_USER_ROLE,

	NUM_OPPRVLG,
};

typedef uint64_t OperationPrivilegeFlag;
const static OperationPrivilegeFlag NONE_PRIVILEGE = 0;
#define FMT_OPPRVLG PRIu64

class OperationPrivilege {
public:
	OperationPrivilege(const UserIdType userId);
	OperationPrivilege(const OperationPrivilegeFlag flags = 0);
	OperationPrivilege(const OperationPrivilege &src);
	virtual ~OperationPrivilege();

	const OperationPrivilegeFlag &getFlags(void) const;
	void setFlags(const OperationPrivilegeFlag flags);
	static const OperationPrivilegeFlag 
	  makeFlag(OperationPrivilegeType type);
	const bool has(OperationPrivilegeType type) const;

	bool operator==(const OperationPrivilege &rhs);

	void setUserId(UserIdType userId);
	UserIdType getUserId(void) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

const static OperationPrivilegeFlag ALL_PRIVILEGES = 
  OperationPrivilege::makeFlag(NUM_OPPRVLG) - 1;

#endif // OperationPrivilege_h
