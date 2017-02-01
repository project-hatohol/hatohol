/*
 * Copyright (C) 2014 Project Hatohol
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
#include "Params.h"
#include "UsedCountable.h"
#include "UsedCountablePtr.h"
#include "OperationPrivilege.h"

/**
 * This class provides a function to share information for data query
 * among DataQueryOption and its subclasses.
 */
class DataQueryContext : public UsedCountable {
public:
	DataQueryContext(const UserIdType &UserId);

	void setUserId(const UserIdType &userId);
	void setFlags(const OperationPrivilegeFlag &flags);
	const OperationPrivilege &getOperationPrivilege(void) const;
	const ServerHostGrpSetMap &getServerHostGrpSetMap(void);
	bool isValidServer(const ServerIdType &serverId);
	const ServerIdSet &getValidServerIdSet(void);

protected:
	// To avoid an instance from being crated on a stack.
	virtual ~DataQueryContext();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

typedef UsedCountablePtr<DataQueryContext> DataQueryContextPtr;

