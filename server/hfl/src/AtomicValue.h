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

#ifndef AtomicValue_h
#define AtomicValue_h

namespace hfl {

template<typename T>
class AtomicValue {
public:
	AtomicValue(const T &initValue)
	{
		m_value = initValue;
	}

	AtomicValue(void)
	: m_value()
	{
	}

	T get(void) const
	{
		return __sync_fetch_and_add(const_cast<T *>(&m_value), 0);
	}

	void set(const T &newVal)
	{
		__sync_bool_compare_and_swap(&m_value, m_value, newVal);
	}

	T add(const T &val)
	{
		return __sync_add_and_fetch(&m_value, val);
	}

	T sub(const T &val)
	{
		return __sync_sub_and_fetch(&m_value, val);
	}

	const T &operator=(const T &rhs)
	{
		set(rhs);
		return rhs;
	}

	operator T() const
	{
		return get();
	}

private:
	volatile T m_value;
};

} // namespace hfl

#endif // AtomicValue_h

