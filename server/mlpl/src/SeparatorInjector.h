/*
 * Copyright (C) 2014 Project Hatohol
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

namespace mlpl {

class SeparatorInjector {
public:
	SeparatorInjector(const char *separator);
	void clear(void);

	/**
	 * Insert the separator at the tail of the string if needed.
	 *
	 * The first one time after the instance is created or
	 * clear() is called, the string is not changed at all.
	 * Otherwise, the separator is inserted at the tail of the string.
	 *
	 * @param str A string to be inserted.
	 */
	void operator()(std::string &str);

private:
	bool        m_first;
	const char *m_separator;
};

} // namespace mlpl

