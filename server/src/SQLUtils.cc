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

#include <cstring>
#include "SQLUtils.h"
#include "SQLProcessorTypes.h"
#include "HatoholException.h"
using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemDataPtr SQLUtils::createFromString(const char *str, SQLColumnType type)
{
	ItemData *itemData = NULL;
	bool strIsNull = false;
	if (!str)
		strIsNull = true;

	switch (type) {
	case SQL_COLUMN_TYPE_INT:
		if (strIsNull)
			itemData = new ItemInt(0, ITEM_DATA_NULL);
		else
			itemData = new ItemInt(atoi(str));
		break;
	case SQL_COLUMN_TYPE_BIGUINT:
		if (strIsNull) {
			itemData = new ItemUint64(0, ITEM_DATA_NULL);
		} else {
			uint64_t val;
			sscanf(str, "%" PRIu64, &val);
			itemData = new ItemUint64(val);
		}
		break;
	case SQL_COLUMN_TYPE_VARCHAR:
	case SQL_COLUMN_TYPE_CHAR:
	case SQL_COLUMN_TYPE_TEXT:
		if (strIsNull)
			itemData = new ItemString("", ITEM_DATA_NULL);
		else
			itemData = new ItemString(str);
		break;
	case SQL_COLUMN_TYPE_DOUBLE:
		if (strIsNull)
			itemData = new ItemDouble(0, ITEM_DATA_NULL);
		else
			itemData = new ItemDouble(atof(str));
		break;
	case SQL_COLUMN_TYPE_DATETIME:
	{ // brace is needed to avoid the error: jump to case label
		if (strIsNull) {
			itemData = new ItemInt(0, ITEM_DATA_NULL);
			break;
		}

		struct tm tm;
		memset(&tm, 0, sizeof(tm));
		int numVal = sscanf(str,
		                    "%04d-%02d-%02d %02d:%02d:%02d",
		                    &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
		                    &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
		static const int EXPECT_NUM_VAL = 6;
		if (numVal != EXPECT_NUM_VAL) {
			MLPL_WARN(
			  "Probably, parse of the time failed: %d, %s\n",
			  numVal, str);
		}
		tm.tm_year -= 1900;
		tm.tm_mon--; // tm_mon is counted from 0 in POSIX time APIs.
		time_t time = mktime(&tm);
		itemData = new ItemInt((int)time);
		break;
	}
	case NUM_SQL_COLUMN_TYPES:
	default:
		THROW_HATOHOL_EXCEPTION("Unknown column type: %d\n", type);
	}
	return itemData;
}
