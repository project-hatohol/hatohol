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

#ifndef EventSemaphore_h
#define EventSemaphore_h

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

#endif // EventSemaphore_h

