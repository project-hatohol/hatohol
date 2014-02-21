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
#include <list>
#include "Params.h"
#include "OperationPrivilege.h"

class DataQueryOption : public OperationPrivilege {
public:
	static const size_t NO_LIMIT;
	enum SortDirection {
		SORT_DONT_CARE,
		SORT_ASCENDING,
		SORT_DESCENDING,
	};
	struct SortOrder {
		std::string columnName;
		SortDirection direction;
		SortOrder(const std::string &columnName,
			  const SortDirection &direction);
	};
	typedef std::list<SortOrder> SortOrderList;
	typedef std::list<SortOrder>::iterator SortOrderListIterator;
	typedef std::list<SortOrder>::const_iterator SortOrderListConstIterator;

	DataQueryOption(UserIdType userId = INVALID_USER_ID);
	DataQueryOption(const DataQueryOption &src);
	virtual ~DataQueryOption();

	bool operator==(const DataQueryOption &rhs);

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
	 * Set a list of SortOrder to build "ORDER BY" statement.
	 *
	 * @param sortOrderList A list of SortOrder.
	 */
	void setSortOrderList(const SortOrderList &sortOrderList);

	/**
	 * Set a SortOrder to build "ORDER BY" statement.
	 * The list of SortOrder which is currently set to this object will be
	 * cleared.
	 *
	 * @param sortOrder A SortOrder.
	 */
	void setSortOrder(const SortOrder &sortOrder);

	/**
	 * Get the list of SortOrder.
	 *
	 * @return The list of SortOrder which is currently set to this object.
	 */
	const SortOrderList &getSortOrderList(void) const;

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
	 * Set the offset of the returned elements.
	 *
	 * @param offset A offset number of returned elements.
	 */
	void setOffset(size_t offset);

	/**
	 * Get the offset number of returned elements.
	 *
	 * @return A offset number of returned elements.
	 */
	size_t getOffset(void) const;

	/**
	 * Get a string for 'where section' of an SQL statement.
	 *
	 * @return a string for 'where' in an SQL statment.
	 */
	virtual std::string getCondition(void) const;

	/**
	 * Get a string for 'order by' section of an SQL statement.
	 *
	 * @return a string for 'order by' in an SQL statment.
	 */
	virtual std::string getOrderBy();

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DataQueryOption_h
