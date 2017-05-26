/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include <list>
#include <ReadWriteLock.h>

/* TODO: Should inherit UsedCountable */
struct ClosureBase
{
	virtual ~ClosureBase(void) {};
	virtual bool operator==(const ClosureBase &closure) {
		// dummy
		return false;
	};
};

struct Closure0 : public ClosureBase
{
	virtual ~Closure0(void) {};
	virtual void operator()(void) = 0;
};

template<typename A>
struct Closure1 : public ClosureBase
{
	virtual ~Closure1(void) {};
	virtual void operator()(const A &arg) = 0;
};

template<class T>
struct ClosureTemplate0 : public Closure0
{
	typedef void (T::*callback)(Closure0 *closure);

	ClosureTemplate0(T *receiver, callback func)
	:m_receiver(receiver), m_func(func)
	{
	}

	virtual void operator()(void) override
	{
		(m_receiver->*m_func)(this);
	}

	virtual bool operator==(const ClosureBase &closureBase) override
	{
		const ClosureTemplate0 *closure
		  = dynamic_cast<const ClosureTemplate0 *>(&closureBase);
		return (m_receiver == closure->m_receiver) &&
		       (m_func == closure->m_func);
	}

	T *m_receiver;
	callback m_func;
};

template<class T, typename A>
struct ClosureTemplate1 : public Closure1<A>
{
	typedef void (T::*callback)(Closure1<A> *closure,
				    const A &arg);

	ClosureTemplate1 (T *receiver, callback func)
	:m_receiver(receiver), m_func(func)
	{
	}

	virtual void operator()(const A &arg) override
	{
		(m_receiver->*m_func)(this, arg);
	}

	virtual bool operator==(const ClosureBase &closureBase) override
	{
		const ClosureTemplate1 *closure
		  = dynamic_cast<const ClosureTemplate1 *>(&closureBase);
		return (m_receiver == closure->m_receiver) &&
		       (m_func == closure->m_func);
	}

	T *m_receiver;
	callback m_func;
};

struct SignalBase
{
	void connect(ClosureBase *closure)
	{
		m_rwlock.writeLock();
		m_closures.push_back(closure);
		m_rwlock.unlock();
	}
	void disconnect(ClosureBase *closure)
	{
		std::list<ClosureBase *>::iterator it;
		m_rwlock.writeLock();
		for (it = m_closures.begin(); it != m_closures.end();) {
			if (*closure == *(*it)) {
				delete (*it);
				it = m_closures.erase(it);
			} else {
				++it;
			}
		}
		m_rwlock.unlock();
	}
	virtual void clear(void)
	{
		m_rwlock.writeLock();
		std::list<ClosureBase *>::iterator it;
		for (it = m_closures.begin(); it != m_closures.end(); ++it) {
			ClosureBase *closure = *it;
			delete closure;
		}
		m_closures.clear();
		m_rwlock.unlock();
	}

	std::list<ClosureBase *> m_closures;
	mlpl::ReadWriteLock m_rwlock;
};

struct Signal0 : public SignalBase
{
	void connect(Closure0 *closure)
	{
		SignalBase::connect(closure);
	}
	void disconnect(Closure0 *closure)
	{
		SignalBase::disconnect(closure);
	}
	virtual void operator()(void)
	{
		m_rwlock.readLock();
		std::list<ClosureBase *>::iterator it;
		for (it = m_closures.begin(); it != m_closures.end(); ++it) {
			Closure0 *closure
				= dynamic_cast<Closure0 *>(*it);
			(*closure)();
		}
		m_rwlock.unlock();
	}
};

template<typename A>
struct Signal1 : public SignalBase
{
	void connect(Closure1<A> *closure)
	{
		SignalBase::connect(closure);
	}
	void disconnect(Closure1<A> *closure)
	{
		SignalBase::disconnect(closure);
	}
	virtual void operator()(const A &arg)
	{
		m_rwlock.readLock();
		std::list<ClosureBase *>::iterator it;
		for (it = m_closures.begin(); it != m_closures.end(); ++it) {
			Closure1<A> *closure
				= dynamic_cast<Closure1<A> *>(*it);
			(*closure)(arg);
		}
		m_rwlock.unlock();
	}
};

