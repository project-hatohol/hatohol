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
#include <cstdio>
#include "Utils.h"

template<class T>
class UsedCountablePtr {
public:
	UsedCountablePtr(void) : m_data(NULL) {
	}

	UsedCountablePtr(const UsedCountablePtr<T> &itemPtr)
	: m_data(itemPtr.m_data) {
		if (m_data)
			m_data->ref();
	}

	UsedCountablePtr(T *data, bool doRef = true)
	: m_data(data) {
		if (doRef && m_data)
			m_data->ref();
	}

	virtual ~UsedCountablePtr() {
		if (m_data) {
			const_cast<T *>(m_data)->unref();
		}
	}

	T *operator->() const {
		return m_data;
	}

	T &operator*() const {
		Utils::assertNotNull(m_data);
		return *m_data;
	}

	operator T *() const {
		return m_data;
	}

	UsedCountablePtr<T> &operator=(T *data) {
		return substitute(data);
	}

	UsedCountablePtr<T> &operator=(UsedCountablePtr<T> &ptr) {
		return substitute(ptr.m_data);
	}

	bool hasData(void) const {
		return (m_data != NULL);
	}

protected:

private:
	T *m_data;

	UsedCountablePtr<T> &substitute(T *data) {
		if (m_data == data)
			return *this;
		if (m_data)
			m_data->unref();
		m_data = data;
		if (m_data)
			m_data->ref();
		return *this;
	}
};

