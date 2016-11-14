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

#include "OperationPrivilege.h"
#include "DBTablesUser.h"
#include "ThreadLocalDBCache.h"
using namespace mlpl;

const OperationPrivilegeFlag OperationPrivilege::NONE_PRIVILEGE = 0;
const OperationPrivilegeFlag OperationPrivilege::ALL_PRIVILEGES =
  OperationPrivilege::makeFlag(NUM_OPPRVLG) - 1ULL;;

struct OperationPrivilege::Impl {
	UserIdType userId;
	uint64_t flags;

	Impl(void)
	: userId(INVALID_USER_ID),
	  flags(0)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
OperationPrivilege::OperationPrivilege(const UserIdType &userId)
: m_impl(new Impl())
{
	setUserId(userId);
}

OperationPrivilege::OperationPrivilege(const OperationPrivilegeFlag &flags)
: m_impl(new Impl())
{
	m_impl->flags = flags;
}

OperationPrivilege::OperationPrivilege(const OperationPrivilegeType &type)
: m_impl(new Impl())
{
	m_impl->flags = makeFlag(type);
}

OperationPrivilege::OperationPrivilege(const OperationPrivilege &src)
: m_impl(new Impl())
{
	*m_impl = *src.m_impl;
}

OperationPrivilege::~OperationPrivilege()
{
}

const OperationPrivilegeFlag &OperationPrivilege::getFlags(void) const
{
	return m_impl->flags;
}

void OperationPrivilege::addFlag(OperationPrivilegeFlag &flags,
				 const OperationPrivilegeType &type)
{
	flags |= (1 << type);
}

void OperationPrivilege::removeFlag(OperationPrivilegeFlag &flags,
				    const OperationPrivilegeType &type)
{
	flags &= ~(1 << type);
}

void OperationPrivilege::add(const OperationPrivilegeType &type)
{
	addFlag(m_impl->flags, type);
}

void OperationPrivilege::remove(const OperationPrivilegeType &type)
{
	removeFlag(m_impl->flags, type);
}

void OperationPrivilege::setFlags(const OperationPrivilegeFlag &flags)
{
	m_impl->flags = flags;
}

OperationPrivilegeFlag OperationPrivilege::makeFlag(
  const OperationPrivilegeType &type)
{
	return (1ULL << type);
}

bool OperationPrivilege::has(const OperationPrivilegeType &type) const
{
	const OperationPrivilegeFlag flag = makeFlag(type);
	return (m_impl->flags & flag);
}

bool OperationPrivilege::operator==(const OperationPrivilege &rhs)
{
	if (m_impl->userId != rhs.m_impl->userId)
		return false;
	if (m_impl->flags != rhs.m_impl->flags)
		return false;
	return true;
}

void OperationPrivilege::setUserId(const UserIdType &userId)
{
	m_impl->userId = userId;
	m_impl->flags = 0;

	if (m_impl->userId == INVALID_USER_ID)
		return;

	if (m_impl->userId == USER_ID_SYSTEM) {
		setFlags(ALL_PRIVILEGES);
		return;
	}

	UserInfo userInfo;
	ThreadLocalDBCache cache;
	if (!cache.getUser().getUserInfo(userInfo, userId)) {
		MLPL_ERR("Failed to getUserInfo(): userId: "
		         "%" FMT_USER_ID "\n", userId);
		m_impl->userId = INVALID_USER_ID;
		return;
	}
	setFlags(userInfo.flags);
}

UserIdType OperationPrivilege::getUserId(void) const
{
	return m_impl->userId;
}
