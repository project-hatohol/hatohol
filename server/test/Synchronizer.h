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
#include <glib.h>
#include <Mutex.h>

class Synchronizer {
	mlpl::Mutex *m_mutex;
public:
	Synchronizer(void);
	virtual ~Synchronizer();
	void lock(void);
	void unlock(void);
	void reset(void);
	bool trylock(void);
	bool isLocked(void);
	void wait(void);
};

