/*
 * Copyright (C) 2013 Project Hatohol
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
#include <cstdlib>
#include <stdint.h>
#include <string>
#include <stdexcept>

namespace mlpl {

static const size_t SMBUF_CURR_INDEX = SIZE_MAX;

class SmartBufferException {
};

class SmartBuffer {
	size_t   m_index;
	uint8_t *m_buf;
	size_t   m_size;
	size_t   m_watermark;
public:
	struct StringHeader {
		uint32_t size;
		int32_t  offset;
	} __attribute__((__packed__));

	SmartBuffer(void);
	SmartBuffer(size_t size);
	SmartBuffer(const SmartBuffer &smartBuffer);
	virtual ~SmartBuffer();
	const SmartBuffer &operator=(const SmartBuffer &rhs);
	operator const char *() const;
	operator char *() const;
	operator uint8_t *() const;
	size_t index(void) const;
	size_t size(void) const;
	size_t remainingSize(void) const;

	/**
	 * Get the watermark.
	 * Watermark is a position at which data is stored.
	 * If resetIndexDeep() or alloc() with resetIndexDeep = true is
	 * called, the watermakr is set to 0.
	 * setAt() methods doesn't affects the watermark. 
	 *
	 * For example,
	 *
	 * SmartBuffer sbuf;
	 * sbuf.add8(3);
	 * sbuf.add32(3);
	 * size_t a = sbuf.watermark(); // 'a' is 5;
	 *
	 * sbuf.resetDeepIndex();
	 * size_t b = sbuf.watermark(); // 'b' is 0;
	 * sbuf.add16(3);
	 * size_t c = sbuf.watermark(); // 'c' is 2;
	 *
	 * sbuf.atSet(10, 3);
	 * size_t d = sbuf.watermark(); // 'c' is 14;
	 *
	 * @return A position of watermark.
	 */
	size_t watermark(void) const;
	void alloc(size_t size, bool resetIndexDeep = true);

	/**
	 * Ensure that the remaining buffer size is large than the specified
	 * size. The remaing size means the difference between the tail of the
	 * buffer and the current index position.
	 * After this method is called, the buffer address might be changed.
	 * Therefore a pointer obtained by getPointer() should not be used
	 * after calling this method.
	 *
	 * @param size A required remaining size.
	 */
	void ensureRemainingSize(size_t size);

	void resetIndex(void);
	void setIndex(const size_t &index = 0);

	/**
	 * reset both the index and the watermark.
	 */
	void resetIndexDeep(void);

	// 'add' families write data at the current index without the boundary
	// check. You must allocate enough buffer before you use these
	// functions.
	void add8(uint8_t val);
	void add16(uint16_t val);
	void add32(uint32_t val);
	void add64(uint64_t val);
	void add(const void *src, size_t len);
	void addZero(size_t size);

	/**
	 * Add a string size followed by the content.
	 *
	 * @tparam T   A type of the size such as uint8_t and uint16_t
	 * @param  str A message to be added.
	 */
	template<typename T>
	void add(const std::string &str)
	{
		const size_t len = str.size();
		const size_t maxLen = (1 << (8 * sizeof(T))) - 1;
		if (len > maxLen) {
			throw std::range_error(
			        "The length of the string is too big.");
		}
		add(&len, sizeof(T));
		add(str.c_str(), len);
	}

	/**
	 * Add a string to the buffer with the size and the location.
	 *
	 * This method writes an unsigned 32bit string length and a signed
	 * 32bit offset of the given string.
	 * Then it writes the string body at the specified position given by
	 * the paramter 'bodyIndex'. Although the NULL terminator of the string
	 * is copied, the written length doesn't include the terminator.
	 * After calling this method, the current position moves to the next
	 * to the length and the offset fields.
	 *
	 * @param str       A string to be written.
	 *
	 * @param bodyIndex
	 * A position where the string body is written from the top of
	 * the buffer.
	 *
	 * @return
	 * The offset next to the written string from the top of the buffer.
	 */
	size_t insertString(const std::string &str, const size_t &bodyIndex);

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

	/**
	 * Get a pointer of the given type.
	 *
	 * @param index
	 * An index of the buffer that is pointed by the returned value.
	 * If this parameter is SMBUF_CURR_INDEX, the current index is used.
	 *
	 * @return A pointer of the type: T.
	 */
	template <typename T>
	T *getPointer(const size_t &index = SMBUF_CURR_INDEX) const {
		size_t headIndex =
		  (index == SMBUF_CURR_INDEX) ? m_index : index;
		return reinterpret_cast<T *>(&m_buf[headIndex]);
	}

	template <typename T>
	T getValue(const size_t &index = SMBUF_CURR_INDEX) const {
		return *getPointer<T>(index);
	}

	template <typename T> T *getPointerAndIncIndex(void) {
		T *ptr = getPointer<T>();
		m_index += sizeof(T);
		return ptr;
	}

	template <typename T> T getValueAndIncIndex(void) {
		return *getPointerAndIncIndex<T>();
	}

	template <typename T>
	std::string getString(const size_t &index = SMBUF_CURR_INDEX) const {
		size_t pos = (index == SMBUF_CURR_INDEX) ? m_index : index;
		T len = *getPointer<T>(pos);
		return std::string(getPointer<char>(pos + sizeof(T)), len);
	}

	template <typename T> std::string getStringAndIncIndex(void)
	{
		std::string s = getString<T>();
		m_index += (sizeof(T) + s.size());
		return s;
	}

	/**
	 * Read a string with the given length and the offset.
	 * After this method is called, the current index is not changed.
	 *
	 * @param header A point to StringHeader region in the buffer.
	 * @return a read string.
	 */
	std::string extractString(const StringHeader *header);

	/**
	 * Read a string with the length and the offset at the current index,
	 * After this method is called, the current index is incremented.
	 *
	 * @return a read string.
	 */
	std::string extractStringAndIncIndex(void);

	/**
	 * Make a new SmartBuffer instance on the heap and the created instance
	 * takes over the content of this buffer. After the call of this
	 * function, the buffer is cleared.
	 *
	 * @return
	 * A newly created SmartBuffer instance. It must be freed with 'delete',
	 * when it's no longer used.
	 *
	 */
	SmartBuffer *takeOver(void);

	/**
	 * Hand over contents of this SmartBuffer instance.
	 * After the call of this methodo, the buffer is cleared.
	 *
	 * @param dest
	 * A destination buffer instance. The contents of it is overwritten.
	 */
	void handOver(SmartBuffer &dest);


protected:
	/**
	 * Update the warter mark if the given index is beyond the current
	 * warter mark.
	 *
	 * @param index An index to be checked.
	 */
	void setWatermarkIfNeeded(const size_t &index);

	void setWatermarkIfNeeded(void) {
		if (m_index > m_watermark)
			m_watermark = m_index;
	}

	template <typename T> void addTemplate(T val) {
		*reinterpret_cast<T *>(&m_buf[m_index]) = val;
		m_index += sizeof(T);
		setWatermarkIfNeeded();
	}

	template <typename T> void addExTemplate(T val) {
		ensureRemainingSize(sizeof(T));
		addTemplate<T>(val);
	}
};

} // namespace mlpl

