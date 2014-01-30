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

#ifndef ItemGroupStream_h 
#define ItemGroupStream_h

#include <string>
#include "ItemGroup.h"

class ItemGroupStream {
public:
	ItemGroupStream(const ItemGroup *itemGroup);

	/**
	 * Get a ItemData instance at the current position.
	 *
	 * This method doesn't move the stream position.
	 *
	 * @return a pointer of ItemData instances.
	 */
	const ItemData *getItem(void) const;

	/**
	 * Read a value of the current ItemData with casting.
	 *
	 * This method forwards the stream position.
	 *
	 * @param NATIVE_TYPE A native type of ItemData.
	 *
	 * @return a value of the current ItemData.
	 */
	template <typename NATIVE_TYPE>
	NATIVE_TYPE read(void)
	{
		NATIVE_TYPE val;
		*this >> val;
                return val;
	}

	/**
	 * Read a value of the current ItemData with casting.
	 *
	 * This method forwards the stream position.
	 *
	 * @param NATIVE_TYPE A native type of ItemData.
	 * @param CAST_TYPE   A type of returned value.
	 *
	 * @return a casted value.
	 */
	template <typename NATIVE_TYPE, typename CAST_TYPE>
	CAST_TYPE read(void)
	{
		return static_cast<CAST_TYPE>(read<NATIVE_TYPE>());
	}

	void operator>>(int &rhs)
	{
		substitute<int>(rhs, *this);
	}

	void operator>>(uint64_t &rhs)
	{
		substitute<uint64_t>(rhs, *this);
	}

	void operator>>(std::string &rhs)
	{
		substitute<std::string>(rhs, *this);
	}

protected:
	template <typename T>
	static T &
	substitute(T &lhs, ItemGroupStream &igStream)
	{
		const ItemData *itemData = igStream.getItem();
		igStream.m_index++;
		lhs = static_cast<T>(*itemData);
		return lhs;
	}

private:
	// To keep peformance, we don't use private context.
	const ItemGroup *m_itemGroup;
	size_t           m_index;
};

#endif // ItemGroupStream_h

