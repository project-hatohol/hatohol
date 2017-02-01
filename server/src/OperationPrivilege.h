/*
 * Copyright (C) 2013,2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <string>
#include <memory>
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

	// IncidentTracker, IncidentSender action, CustomIncidentStatus
	OPPRVLG_CREATE_INCIDENT_SETTING,
	OPPRVLG_UPDATE_INCIDENT_SETTING,
	OPPRVLG_DELETE_INCIDENT_SETTING,
	OPPRVLG_GET_ALL_INCIDENT_SETTINGS,

	// Othrer
	OPPRVLG_GET_SYSTEM_INFO,
	OPPRVLG_SYSTEM_OPERATION,

	// SeverityRank
	OPPRVLG_CREATE_SEVERITY_RANK,  // can create SeverityRank
	OPPRVLG_UPDATE_SEVERITY_RANK,  // can update SeverityRank
	OPPRVLG_DELETE_SEVERITY_RANK,  // can delete SeverityRank

	NUM_OPPRVLG,
};

typedef uint64_t OperationPrivilegeFlag;
#define FMT_OPPRVLG PRIu64

class OperationPrivilege {
public:
	const static OperationPrivilegeFlag NONE_PRIVILEGE;
	const static OperationPrivilegeFlag ALL_PRIVILEGES;

	OperationPrivilege(const UserIdType &userId);
	OperationPrivilege(const OperationPrivilegeFlag &flags = 0);
	OperationPrivilege(const OperationPrivilege &src);
	virtual ~OperationPrivilege();

	const OperationPrivilegeFlag &getFlags(void) const;
	void add(const OperationPrivilegeType &type);
	void remove(const OperationPrivilegeType &type);
	void setFlags(const OperationPrivilegeFlag &flags);
	static OperationPrivilegeFlag makeFlag(
	  const OperationPrivilegeType &type);
	static void addFlag(
	  OperationPrivilegeFlag &flags,
	  const OperationPrivilegeType &type);
	static void removeFlag(
	  OperationPrivilegeFlag &flags,
	  const OperationPrivilegeType &type);
	bool has(const OperationPrivilegeType &type) const;

	bool operator==(const OperationPrivilege &rhs);

	void setUserId(const UserIdType &userId);
	UserIdType getUserId(void) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

