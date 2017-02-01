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

#pragma once
#include <string>
#include <list>
#include <stdint.h>
#include <memory>
#include <Params.h>
#include "DBTermCodec.h"

class DBTermCStringProvider {
public:
	DBTermCStringProvider(const DBTermCodec &codec);
	~DBTermCStringProvider();

	const char * operator()(const int &val);
	const char * operator()(const uint64_t &val);
	const char * operator()(const std::string &val);

	const std::list<std::string> &getStoredStringList(void) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

