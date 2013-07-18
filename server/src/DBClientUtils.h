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

#ifndef DBClientUtils_h
#define DBClientUtils_h

#define GET_FROM_GRP(NATIVE_TYPE, ITEM_TYPE, ITEM_GRP, IDX) \
ItemDataUtils::get<NATIVE_TYPE, ITEM_TYPE>(ITEM_GRP->getItemAt(IDX))

#define GET_UINT64_FROM_GRP(ITEM_GRP, IDX) \
GET_FROM_GRP(uint64_t, ItemUint64, ITEM_GRP, IDX)

#define GET_INT_FROM_GRP(ITEM_GRP, IDX) \
GET_FROM_GRP(int, ItemInt, ITEM_GRP, IDX)

#define GET_STRING_FROM_GRP(ITEM_GRP, IDX) \
GET_FROM_GRP(string, ItemString, ITEM_GRP, IDX)

#endif // DBClientUtils_h
