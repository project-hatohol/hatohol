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

namespace mlpl {

class EventSemaphore {
public:
	/**
	 * A constructor of EventSemaphore.
	 *
	 * @param count An initial count of the semaphore.
	 */
	EventSemaphore(const int &count = 1);

	virtual ~EventSemaphore();

	/**
	 * Post a semaphore.
	 *
	 * @return 0 on Succeess, or errno if an error occured.
	 */
	int post(void);

	/**
	 * Wait a semaphore.
	 *
	 * @return 0 on Succeess, or errno if an error occured.
	 */
	int wait(void);

	/**
	 * Get a file descriptor of a eventfd object.
	 *
	 * @return a file descripter.
	 */
	int getEventFd(void) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

} // namespace mlpl

