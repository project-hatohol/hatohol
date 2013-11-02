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
	static const size_t NO_LIMIT;
	enum SortOrder {
		SORT_DONT_CARE,
		SORT_ASCENDING,
		SORT_DESCENDING,
	};

	DataQueryOption(void);
	~DataQueryOption();

	bool operator==(const DataQueryOption &rhs);

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
	 * Get the maximum number of returned elements.
	 *
	 * @return A maximum number of returned elements.
	 */
	size_t getMaximumNumber(void) const;

	/**
	 * Set the sort order of returned elements.
	 *
	 * @param order A sort order.
	 */
	void setSortOrder(SortOrder order);

	/**
	 * Get the sort order of returned elements.
	 *
	 * @return A sort order.
	 */
	SortOrder getSortOrder(void) const;

	/**
	 * Set a start ID.
	 *
	 * @param id A start ID.
	 */
	void setStartId(uint64_t id);

	/**
	 * Get the start ID.
	 *
	 * @return A start ID.
	 */
	uint64_t getStartId(void) const;

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
