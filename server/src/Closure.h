/*
 * Copyright (C) 2013 Project Hatohol
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

#ifndef Closure_h
#define Closure_h

#include <list>
#include <ReadWriteLock.h>

using namespace std;
using namespace mlpl;

struct ClosureBase
{
	ClosureBase(void) {};
	virtual ~ClosureBase(void) {};
	virtual void operator()(void) = 0;
	virtual bool operator==(const ClosureBase &closure) = 0;
};

template<class T>
struct Closure : public ClosureBase
{
	typedef void (T::*callback)(void);

	Closure (T *receiver, callback func)
	:m_receiver(receiver), m_func(func)
	{
	}

	virtual void operator()(void)
	{
		(m_receiver->*m_func)();
	}

	virtual bool operator==(const ClosureBase &closureBase)
	{
		const Closure *closure
		  = dynamic_cast<const Closure*>(&closureBase);
		return (m_receiver == closure->m_receiver) &&
		       (m_func == closure->m_func);
	}

	T *m_receiver;
	callback m_func;
};

typedef list<ClosureBase *>             ClosureBaseList;
typedef ClosureBaseList::iterator       ClosureBaseIterator;
typedef ClosureBaseList::const_iterator ClosureBaseListConstIterator;

struct Signal
{
	Signal(void);
	virtual ~Signal(void);
	virtual void connect(ClosureBase *closure);
	virtual void disconnect(ClosureBase *closure);
	virtual void clear(void);
	virtual void operator()(void);
	ClosureBaseList m_closures;
	ReadWriteLock m_rwlock;
};

#endif // Closure_h
