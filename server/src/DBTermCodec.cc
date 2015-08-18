/*
 * Copyright (C) 2014-2015 Project Hatohol
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

#include <StringUtils.h>
#include <inttypes.h>
#include "DBTermCodec.h"
using namespace std;
using namespace mlpl;

string DBTermCodec::enc(const int &val) const
{
	return StringUtils::sprintf("%d", val);
}

string DBTermCodec::enc(const uint64_t &val) const
{
	return StringUtils::sprintf("%" PRIu64, val);
}

string DBTermCodec::enc(const string &val) const
{
	// Find quotations that might cause an SQL injection
	vector<const char *> quotPosArray;

	for (const char *ch = val.c_str(); *ch; ch++) {
		if (*ch == '\'')
			quotPosArray.push_back(ch);
	}

	// Make a string in which quotations are escaped
	const size_t sizeQuationsAtBothEnds = 2;
	string str;
	str.reserve(val.size() + quotPosArray.size() + sizeQuationsAtBothEnds);
	str += '\'';
	const char *head = val.c_str();
	for (size_t i = 0; i < quotPosArray.size(); i++) {
		const char *tail = quotPosArray[i];
		const size_t len = tail - head + 1;
		str.append(head, len);
		str += '\'';
		head = tail + 1;
	}
	str.append(head);
	str += '\'';
	return str;
}
