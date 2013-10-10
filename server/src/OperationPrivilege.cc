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

#include "OperationPrivilege.h"

struct OperationPrivilege::PrivateContext {
	uint64_t flags;

	PrivateContext(void)
	: flags(0)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
OperationPrivilege::OperationPrivilege(const OperationPrivilegeFlag &flags)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	m_ctx->flags = flags;
}

OperationPrivilege::~OperationPrivilege()
{
	if (m_ctx)
		delete m_ctx;
}

const OperationPrivilegeFlag &OperationPrivilege::getFlags(void) const
{
	return m_ctx->flags;
}

void OperationPrivilege::setFlags(const OperationPrivilegeFlag &flags)
{
	m_ctx->flags = flags;
}

const OperationPrivilegeFlag
OperationPrivilege::makeFlag(OperationPrivilegeType type)
{
	return (1 << type);
}

const bool OperationPrivilege::has(OperationPrivilegeType type) const
{
	const OperationPrivilegeFlag flag = makeFlag(type);
	return (m_ctx->flags & flag);
}

