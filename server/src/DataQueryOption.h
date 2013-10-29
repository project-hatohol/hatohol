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

#ifndef DataQueryOption_h
#define DataQueryOption_h

#include <string>
#include "Params.h"
#include "OperationPrivilege.h"

class DataQueryOption : public OperationPrivilege {
public:
	static const size_t NO_LIMIT = 0;

	DataQueryOption(void);
	~DataQueryOption();

	void setUserId(UserIdType userId);
	UserIdType getUserId(void) const;

	/**
	 * Set the maximum number of the returned elements.
	 *
	 * @param maximum A maximum number. If NO_LIMIT is given, all data
	 * are returned.
	 */
	virtual void setMaximumNumber(size_t maximum);

	/**
	 * Get a string for 'where section' of an SQL statement.
	 *
	 * @return a string for 'where' in an SQL statment.
	 */
	virtual std::string getCondition(void) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DataQueryOption_h
