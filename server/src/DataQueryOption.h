/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include <vector>
#include "Params.h"
#include "DataQueryContext.h"
#include "DBTermCodec.h"

class DataQueryOption {
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
	typedef std::vector<SortOrder> SortOrderVect;
	typedef std::vector<SortOrder>::iterator SortOrderVectIterator;
	typedef std::vector<SortOrder>::const_iterator SortOrderVectConstIterator;

	struct GroupBy {
		std::string columnName;
		GroupBy(const std::string &columnName);
	};
	typedef std::vector<GroupBy> GroupByVect;

	DataQueryOption(const UserIdType &userId = INVALID_USER_ID);
	DataQueryOption(DataQueryContext *dataQueryContext);
	DataQueryOption(const DataQueryOption &src);
	virtual ~DataQueryOption();

	bool operator==(const DataQueryOption &rhs);

	DataQueryContext &getDataQueryContext(void) const;
	void setDBTermCodec(const DBTermCodec *optionTermGenerator);
	virtual const DBTermCodec *getDBTermCodec(void) const;

	/**
	 * Check if the server is registered.
	 *
	 * Hatohol doesn't delete data immediately after the monitoring server
	 * is removed. So this method helps some routines check if the server
	 * of data is being registered or not.
	 *
	 * @param serverId A server ID.
	 * @return
	 * true when the given ID is registered in the Config DB.
	 * Otherwise false is returned.
	 */
	bool isValidServer(const ServerIdType &serverId) const;

	/**
	 * Set a user ID.
	 *
	 * @param userId A user ID.
	 */
	void setUserId(const UserIdType &userId);
	void setFlags(const OperationPrivilegeFlag &flags);

	UserIdType getUserId(void) const;
	bool has(const OperationPrivilegeType &type) const;
	operator const OperationPrivilege &() const;

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
	 * Set a vector of SortOrder to build "ORDER BY" statement.
	 *
	 * @param sortOrderVect A vector of SortOrder.
	 */
	void setSortOrderVect(const SortOrderVect &sortOrderVect);

	/**
	 * Set a SortOrder to build "ORDER BY" statement.
	 * The list of SortOrder which is currently set to this object will be
	 * cleared.
	 *
	 * @param sortOrder A SortOrder.
	 */
	void setSortOrder(const SortOrder &sortOrder);

	/**
	 * Get the vector of SortOrder.
	 *
	 * @return The vector of SortOrder which is currently set to this object.
	 */
	const SortOrderVect &getSortOrderVect(void) const;

	/**
	 * Set a vector of GroupBy to build "GROUP BY" statement.
	 *
	 * @param groupByVect A vector of GroupBy.
	 */
	void setGroupByVect(const GroupByVect &groupByVect);

	/**
	 * Set a GroupBy to build "GROUP BY" statement.
	 * The list of GroupBy which is currently set to this object will be
	 * cleared.
	 *
	 * @param groupBy A GroupBy.
	 */
	void setGroupBy(const GroupBy &groupBy);

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
	 * Get a string for 'where clause' of an SQL statement.
	 *
	 * @return a string for 'where' in an SQL statment.
	 */
	virtual std::string getCondition(void) const;

	/**
	 * Get a string for 'order by' clause of an SQL statement.
	 *
	 * @return a string for 'order by' in an SQL statment.
	 */
	virtual std::string getOrderBy(void) const;

	/**
	 * Get a string for 'group by' clause of an SQL statement.
	 *
	 * @return a string for 'group by' in an SQL statment.
	 */
	virtual std::string getGroupBy(void) const;

	/**
	 * Set the flag always to use the table name.
	 *
	 * @param enable A flag to enable the feature.
	 */
	void setTableNameAlways(const bool &enable = true) const;

	/**
	 * Get the flag always to use the table name.
	 *
	 * @param enable A flag to enable the feature.
	 */
	bool getTableNameAlways(void) const;

protected:
	enum AddConditionType {
		ADD_TYPE_AND,
		ADD_TYPE_OR,
	};

	/**
	 * Append a condtion string.
	 *
	 * @param currCondition
	 * A current condition string or an empty string.
	 *
	 * @param addedCondition
	 * An added conditioin sting. This string is added at tail. If this
	 * string is empty, currCondition is not changed.
	 *
	 * @param type
	 * An AddedConditioinType.
	 *
	 * @param useParenthesis
	 * If this option is true, the added condition string is wrapped
	 * by parenthesis.
	 */
	static void addCondition(std::string &currCondition,
	                         const std::string &addedCondition,
	                         const AddConditionType &type = ADD_TYPE_AND,
	                         const bool &useParenthesis = false);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

