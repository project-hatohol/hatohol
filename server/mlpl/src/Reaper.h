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

#ifndef Reaper_h
#define Reaper_h

namespace mlpl {

template<typename T>
class Reaper {
public:
	Reaper(void)
	: m_obj(NULL),
	  m_destroyFunc(NULL)
	{
	}

	Reaper(T *obj, void (*destroyFunc)(T *))
	: m_obj(obj),
	  m_destroyFunc(destroyFunc)
	{
	}

	virtual ~Reaper()
	{
		if (m_obj)
			(*m_destroyFunc)(m_obj);
	}

	void deactivate(void)
	{
		m_obj = NULL;
	}

	bool set(T *obj, void (*destroyFunc)(T *))
	{
		if (m_obj || m_destroyFunc)
			return false;
		m_obj = obj;
		m_destroyFunc = destroyFunc;
		return true;
	}

protected:
	T *m_obj;

private:
	void (*m_destroyFunc)(T *);
};

template<typename T>
class CppReaper : public Reaper<T> {
public:
	CppReaper(void)
	{
	}

	CppReaper(T *obj)
	: Reaper<T>(obj, NULL)
	{
	}

	virtual ~CppReaper()
	{
		if (Reaper<T>::m_obj) {
			delete Reaper<T>::m_obj;
			Reaper<T>::m_obj = NULL;
		}
	}

	bool set(T *obj)
	{
		return Reaper<T>::set(obj, NULL);
	}
};

} // namespace mlpl

#endif // Reaper_h

