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

#pragma once
#include <AtomicValue.h>

class UsedCountable {
public:
	void ref(void) const;
	void unref(void) const;
	int getUsedCount(void) const;

	static void unref(UsedCountable *countable);

protected:
	UsedCountable(const int &initialUsedCount = 1);
	virtual ~UsedCountable();

private:
	mutable mlpl::AtomicValue<int> m_usedCount;
};

