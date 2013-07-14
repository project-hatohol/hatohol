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

#ifndef SmartBuffer_h
#define SmartBuffer_h

#include <cstdlib>
#include <stdint.h>

namespace mlpl {

class SmartBufferException {
};

class SmartBuffer {
	size_t   m_index;
	uint8_t *m_buf;
	size_t   m_size;
public:
	SmartBuffer(void);
	SmartBuffer(size_t size);
	virtual ~SmartBuffer();
	operator const char *() const;
	operator char *() const;
	operator uint8_t *() const;
	size_t index(void) const;
	size_t size(void) const;
	size_t remainingSize(void) const;
	void alloc(size_t size, bool resetIndex = true);
	void ensureRemainingSize(size_t size);

	void resetIndex(void);

	// 'add' families write data at the current index without the boundary
	// check. You must allocate enough buffer before you use these
	// functions.
	void add8(uint8_t val);
	void add16(uint16_t val);
	void add32(uint32_t val);
	void add64(uint64_t val);
	void add(const void *src, size_t len);
	void addZero(size_t size);

	// 'addEx' families extend buffer if needed. They are useful when
	// the total size of the buffer is unknown. Of course, because
	// the reallocation might be called, performance is less than that of
	// 'add' families.
	void addEx8(uint8_t val);
	void addEx16(uint16_t val);
	void addEx32(uint32_t val);
	void addEx64(uint64_t val);
	void addEx(const void *src, size_t len);

	void incIndex(size_t size);
	void setAt(size_t index, uint32_t val);
	void printBuffer(void);

	template <typename T> T *getPointer(void) {
		return reinterpret_cast<T *>(&m_buf[m_index]);
	}

	template <typename T> T getValue(void) {
		return *getPointer<T>();
	}

	template <typename T> T *getPointerAndIncIndex(void) {
		T *ptr = getPointer<T>();
		m_index += sizeof(T);
		return ptr;
	}

	template <typename T> T getValueAndIncIndex(void) {
		return *getPointerAndIncIndex<T>();
	}

protected:
	template <typename T> void addTemplate(T val) {
		*reinterpret_cast<T *>(&m_buf[m_index]) = val;
		m_index += sizeof(T);
	}

	template <typename T> void addExTemplate(T val) {
		ensureRemainingSize(sizeof(T));
		addTemplate<T>(val);
	}
};

} // namespace mlpl

#endif // SmartBuffer_h
