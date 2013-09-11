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

typedef void (*ReaperDestroyFunc)(void *obj);

template<typename T>
class Reaper {
public:
	Reaper(T *obj, ReaperDestroyFunc destroyFunc = NULL)
	: m_obj(obj),
	  m_destroyFunc(destroyFunc),
	  m_active(true)
	{
	}

	virtual ~Reaper()
	{
		if (!m_active)
			return;
		if (!m_destroyFunc)
			delete m_obj;
		else
			(*m_destroyFunc)(m_obj);
	}

	void deactivate(void)
	{
		m_active = false;
	}

private:
	T *m_obj;
	ReaperDestroyFunc m_destroyFunc;
	bool m_active;
};

} // namespace mlpl

#endif // Reaper_h

