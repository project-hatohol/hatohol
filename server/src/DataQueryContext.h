/*
 * Copyright (C) 2014 Project Hatohol
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

#ifndef DataQueryContext_h
#define DataQueryContext_h

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

protected:
	// To avoid an instance from being crated on a stack.
	virtual ~DataQueryContext();

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

typedef UsedCountablePtr<DataQueryContext> DataQueryContextPtr;

#endif // DataQueryContext_h
