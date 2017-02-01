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
#include <memory>
#include <SmartTime.h>

class StatisticsCounter {
public:
	struct Slot {
		// The effective interval T: startTime <= T < endTime
		mlpl::SmartTime startTime;
		mlpl::SmartTime endTime;
		uint64_t number;
	};

	StatisticsCounter(const int &intervalSec,
	                  const int &intervalNanosec=0);
	virtual ~StatisticsCounter();

	void add(const uint64_t &number);

	/**
	 * Get the statistics.
	 *
	 * @param prevSlot
	 * The data of the previous time slot is stored in this variable.
	 * It should have the data collected in the given interval.
	 *
	 * @param currSlot
	 * The data of the currently used slot is stored. The measured
	 * interval changes depending on the call time. If this parameter
	 * is NULL, no data is stored.
	 */
	void get(Slot *prevSlot, Slot *currSlot = NULL);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

