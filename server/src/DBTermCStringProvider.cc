/*
 * Copyright (C) 2015 Project Hatohol
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

#include "DBTermCStringProvider.h"
using namespace std;

struct DBTermCStringProvider::Impl {
	const DBTermCodec &codec;
	list<string>       strList;

	// methods
	Impl(const DBTermCodec &_codec)
	: codec(_codec)
	{
	}

	template<typename T>
	const char *encAndStore(const T &val)
	{
		strList.push_back(codec.enc(val));
		return strList.back().c_str();
	}
};

DBTermCStringProvider::DBTermCStringProvider(const DBTermCodec &codec)
: m_impl(new Impl(codec))
{
}

DBTermCStringProvider::~DBTermCStringProvider()
{
}

const char * DBTermCStringProvider::operator()(const int &val)
{
	return m_impl->encAndStore<int>(val);
}

const char * DBTermCStringProvider::operator()(const uint64_t &val)
{
	return m_impl->encAndStore<uint64_t>(val);
}

const char * DBTermCStringProvider::operator()(const string &val)
{
	return m_impl->encAndStore<string>(val);
}

const list<string> &DBTermCStringProvider::getStoredStringList(void) const
{
	return m_impl->strList;
}
