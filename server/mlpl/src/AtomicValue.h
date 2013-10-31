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

namespace mlpl {

template<typename T>
class AtomicValue {
public:
	AtomicValue(const T &initValue)
	{
		m_value = initValue;
	}

	AtomicValue(void)
	{
	}

	T get(void)
	{
		static T v = 0;
		return __sync_fetch_and_add(&m_value, &v);
	}

	void set(const T &newVal)
	{
		__sync_val_compare_and_swap(&m_value, m_value, newVal);
	}


private:
	volatile T m_value;
};

} // namespace mlpl

#endif // AtomicValue_h

