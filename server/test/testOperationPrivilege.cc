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

#include <cppcutter.h>
#include "OperationPrivilege.h"

namespace testOperationPrivilege {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
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

void test_getSpecifiedFlags(void)
{
	OperationPrivilege privilege(ALL_PRIVILEGES);
	cppcut_assert_equal(ALL_PRIVILEGES, privilege.getFlags());
}

void test_allPrivileges(void)
{
	OperationPrivilege privilege(ALL_PRIVILEGES);
	cppcut_assert_equal(true, privilege.has(OPPRVLG_CREATE_USER));
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
		  (OperationPrivilegeFlag)(1 << i),
		  OperationPrivilege::makeFlag(type));
	}
}

void test_has(void)
{
	OperationPrivilege privilege;
	cppcut_assert_equal(false, privilege.has(OPPRVLG_CREATE_USER));
}

} // namespace testOperationPrivilege
