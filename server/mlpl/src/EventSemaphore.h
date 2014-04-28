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
	EventSemaphore(const int &count = 1);
	virtual ~EventSemaphore();
	void post(void);
	uint64_t wait(void);
	int getEventFd(void) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

} // namespace mlpl

#endif // EventSemaphore_h

