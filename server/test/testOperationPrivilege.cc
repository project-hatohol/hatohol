/*
 * Copyright (C) 2013 Project Hatohol
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

#include <cppcutter.h>
#include "Hatohol.h"
#include "OperationPrivilege.h"
#include "Helpers.h"
#include "DBTablesTest.h"

namespace testOperationPrivilege {

static void initAndLoadTestUser(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBUser();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_defaultUserId(void)
{
	OperationPrivilege privilege;
	cppcut_assert_equal(INVALID_USER_ID, privilege.getUserId());
}

void test_constructorWithUserId(void)
{
	initAndLoadTestUser();
	const UserIdType userId = searchMaxTestUserId();
	OperationPrivilege privilege(userId);
	cppcut_assert_equal(userId, privilege.getUserId());

	// non existing user ID
	const UserIdType nonExistingUserId = userId + 1;
	OperationPrivilege privilegeNonExistingUser(nonExistingUserId);
	cppcut_assert_equal(INVALID_USER_ID,
	                    privilegeNonExistingUser.getUserId());
}

void test_setGetUserId(void)
{
	initAndLoadTestUser();
	const UserIdType userId = 2;
	OperationPrivilege privilege;
	privilege.setUserId(userId);
	cppcut_assert_equal(userId, privilege.getUserId());
}

void test_setUnknownserId(void)
{
	initAndLoadTestUser();
	const UserIdType userId = 1129;
	OperationPrivilege privilege;
	privilege.setUserId(userId);
	cppcut_assert_equal(INVALID_USER_ID, privilege.getUserId());
}

void test_getDefaultFlags(void)
{
	OperationPrivilege privilege;
	cppcut_assert_equal((OperationPrivilegeFlag)0, privilege.getFlags());
}

void test_copyConstructor(void)
{
	OperationPrivilege privilege;
	privilege.setFlags(OperationPrivilege::makeFlag(OPPRVLG_CREATE_USER));
	OperationPrivilege copied(privilege);
	cppcut_assert_equal(privilege.getFlags(), copied.getFlags());
}

void test_copyConstructorWithUserId(void)
{
	OperationPrivilege privilege;
	privilege.setUserId(USER_ID_SYSTEM);
	OperationPrivilege copied(privilege);
	cppcut_assert_equal(USER_ID_SYSTEM, copied.getUserId());
	cppcut_assert_equal(privilege.getFlags(), copied.getFlags());
}

void test_getSpecifiedFlags(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	cppcut_assert_equal(OperationPrivilege::ALL_PRIVILEGES,
			    privilege.getFlags());
}

void test_allPrivileges(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	cppcut_assert_equal(true, privilege.has(OPPRVLG_CREATE_USER));
}

void test_add(void)
{
	OperationPrivilegeFlag initialFlags = 1 << OPPRVLG_CREATE_USER;
	OperationPrivilegeFlag expectedFlags =
	  (1 << OPPRVLG_CREATE_USER) | (1 << OPPRVLG_UPDATE_USER);
	OperationPrivilege privilege(initialFlags);
	cppcut_assert_equal(initialFlags, privilege.getFlags());
	privilege.add(OPPRVLG_UPDATE_USER);
	cppcut_assert_equal(expectedFlags, privilege.getFlags());
}

void test_remove(void)
{
	OperationPrivilegeFlag initialFlags =
	  (1 << OPPRVLG_CREATE_USER) | (1 << OPPRVLG_UPDATE_USER);
	OperationPrivilegeFlag expectedFlags = 1 << OPPRVLG_CREATE_USER;
	OperationPrivilege privilege(initialFlags);
	cppcut_assert_equal(initialFlags, privilege.getFlags());
	privilege.remove(OPPRVLG_UPDATE_USER);
	cppcut_assert_equal(expectedFlags, privilege.getFlags());
}

void test_addFlag(void)
{
	OperationPrivilegeFlag flags = 1 << OPPRVLG_CREATE_USER;
	OperationPrivilegeFlag expectedFlags =
	  (1 << OPPRVLG_CREATE_USER) | (1 << OPPRVLG_UPDATE_USER);
	OperationPrivilege::addFlag(flags, OPPRVLG_UPDATE_USER);
	cppcut_assert_equal(expectedFlags, flags);
}

void test_removeFlag(void)
{
	OperationPrivilegeFlag flags =
	  (1 << OPPRVLG_CREATE_USER) | (1 << OPPRVLG_UPDATE_USER);
	OperationPrivilegeFlag expectedFlags = 1 << OPPRVLG_CREATE_USER;
	OperationPrivilege::removeFlag(flags, OPPRVLG_UPDATE_USER);
	cppcut_assert_equal(expectedFlags, flags);
}

void test_setFlags(void)
{
	OperationPrivilegeFlag flags =
	  OperationPrivilege::makeFlag(OPPRVLG_CREATE_USER);
	OperationPrivilege privilege;
	privilege.setFlags(flags);
	cppcut_assert_equal(flags, privilege.getFlags());
}

void test_makeFlag(void)
{
	for (size_t i = 0; i < NUM_OPPRVLG; i++) {
		OperationPrivilegeType type =
		  static_cast<OperationPrivilegeType>(i);
		cppcut_assert_equal(
		  (OperationPrivilegeFlag)(1ULL << i),
		  OperationPrivilege::makeFlag(type));
	}
}

void test_has(void)
{
	OperationPrivilege privilege;
	cppcut_assert_equal(false, privilege.has(OPPRVLG_CREATE_USER));
}

void test_operatorEq(void)
{
	initAndLoadTestUser();

	OperationPrivilege lhs;
	OperationPrivilege rhs;
	cppcut_assert_equal(true, lhs == rhs);

	lhs.setUserId(2);
	rhs.setUserId(2);
	lhs.setFlags(OperationPrivilege::makeFlag(OPPRVLG_CREATE_USER));
	rhs.setFlags(OperationPrivilege::makeFlag(OPPRVLG_CREATE_USER));
	cppcut_assert_equal(true, lhs == rhs);
}

void test_operatorEqToSelf(void)
{
	OperationPrivilege lhs;
	cppcut_assert_equal(true, lhs == lhs);
}

void test_operatorEqFalse(void)
{
	OperationPrivilege lhs;
	OperationPrivilege rhs;
	rhs.setFlags(OperationPrivilege::makeFlag(OPPRVLG_CREATE_USER));
	cppcut_assert_equal(false, lhs == rhs);
}

void test_operatorFalseDifferentUserId(void)
{
	initAndLoadTestUser();
	OperationPrivilege lhs;
	OperationPrivilege rhs;
	cppcut_assert_equal(true, lhs == rhs);

	lhs.setUserId(1);
	rhs.setUserId(2);
	cppcut_assert_equal(false, lhs == rhs);
}

} // namespace testOperationPrivilege
