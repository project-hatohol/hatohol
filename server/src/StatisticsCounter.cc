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

#include <Logger.h>
#include <mutex>
#include "StatisticsCounter.h"
#include <cstdio>
using namespace std;
using namespace mlpl;

struct StatisticsCounter::Impl {
private:
	timespec interval;
	mutex slotsMutex;
	Slot slots[2];
	SmartTime currSlotEndTime;

public:
	Impl(const timespec &_interval)
	: interval(_interval)
	{
		initializeSlots(SmartTime(SmartTime::INIT_CURR_TIME));
	}

	void add(const uint64_t &number)
	{
		lock_guard<mutex> lock(slotsMutex);
		fixupSlots();
		getCurrSlot().number += number;
	}

	void get(Slot *prevSlot, Slot *currSlot)
	{
		lock_guard<mutex> lock(slotsMutex);
		fixupSlots();
		*prevSlot = getPrevSlot();
		if (currSlot)
			*currSlot = getCurrSlot();
	}

private:
	Slot &getPrevSlot(void)
	{
		return slots[0];
	}

	Slot &getCurrSlot(void)
	{
		return slots[1];
	}

	void initializeSlots(const SmartTime &now)
	{
		Slot &prevSlot = getPrevSlot();
		Slot &currSlot = getCurrSlot();

		prevSlot.startTime = now;
		prevSlot.startTime -= interval;
		prevSlot.endTime = now;
		prevSlot.number = 0;

		currSlot.startTime = now;
		currSlot.endTime = now;
		currSlot.number = 0;

		currSlotEndTime = now;
		currSlotEndTime += interval;
	}

	void fixupSlots(void)
	{
		SmartTime now = SmartTime(SmartTime::INIT_CURR_TIME);
		if (now < currSlotEndTime) {
			getCurrSlot().endTime = now;
			return;
		}
		SmartTime nextEndTime = currSlotEndTime;
		nextEndTime += interval;
		if (now < nextEndTime) {
			Slot &prevSlot = getPrevSlot();
			Slot &currSlot = getCurrSlot();
			prevSlot = currSlot;
			prevSlot.endTime = currSlotEndTime;

			currSlot.startTime = prevSlot.endTime;
			currSlot.endTime = now;
			currSlot.number = 0;
			currSlotEndTime = nextEndTime;
		} else {
			initializeSlots(now);
		}
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
StatisticsCounter::StatisticsCounter(const int &intervalSec,
                                     const int &intervalNanosec)
: m_impl(new Impl({intervalSec, intervalNanosec}))
{
}

StatisticsCounter::~StatisticsCounter()
{
}

void StatisticsCounter::add(const uint64_t &number)
{
	m_impl->add(number);
}

void StatisticsCounter::get(Slot *prevSlot, Slot *currSlot)
{
	m_impl->get(prevSlot, currSlot);
}
