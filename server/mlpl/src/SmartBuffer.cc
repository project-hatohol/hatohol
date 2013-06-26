/*
 * Copyright (C) 2013 Hatohol Project
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

#include <cstdio>
#include <cstring>
#include <string>
using namespace std;

#include "SmartBuffer.h"
#include "Logger.h"
using namespace mlpl;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SmartBuffer::SmartBuffer(void)
: m_index(0),
  m_buf(NULL),
  m_size(0)
{
}

SmartBuffer::SmartBuffer(size_t size)
: m_index(0),
  m_buf(NULL),
  m_size(0)
{
	alloc(size);
}

SmartBuffer::~SmartBuffer()
{
	if (m_buf)
		free(m_buf);
}

SmartBuffer::operator const char *() const
{
	return reinterpret_cast<const char *>(m_buf);
}

SmartBuffer::operator char *() const
{
	return reinterpret_cast<char *>(m_buf);
}

SmartBuffer::operator uint8_t *() const
{
	return reinterpret_cast<uint8_t *>(m_buf);
}

size_t SmartBuffer::index(void) const
{
	return m_index;
}

size_t SmartBuffer::size(void) const
{
	return m_size;
}

size_t SmartBuffer::remainingSize(void) const
{
	return m_size - m_index;
}

void SmartBuffer::alloc(size_t size, bool resetIndex)
{
	m_buf = static_cast<uint8_t *>(realloc(m_buf, size));
	if (m_buf == NULL)
		throw SmartBufferException();
	if (resetIndex)
		m_index = 0;
	m_size = size;
}

void SmartBuffer::ensureRemainingSize(size_t size)
{
	size_t requiredSize = m_index + size;
	if (requiredSize > m_size)
		alloc(requiredSize, false);
}

void SmartBuffer::resetIndex(void)
{
	m_index = 0;
}

void SmartBuffer::add8(uint8_t val)
{
	addTemplate<uint8_t>(val);
}

void SmartBuffer::add16(uint16_t val)
{
	addTemplate<uint16_t>(val);
}

void SmartBuffer::add32(uint32_t val)
{
	addTemplate<uint32_t>(val);
}

void SmartBuffer::add64(uint64_t val)
{
	addTemplate<uint64_t>(val);
}

void SmartBuffer::add(const void *src, size_t len)
{
	memcpy(&m_buf[m_index], src, len);
	m_index += len;
}

void SmartBuffer::addZero(size_t size)
{
	memset(&m_buf[m_index], 0, size);
	m_index += size;
}

void SmartBuffer::addEx8(uint8_t val)
{
	addExTemplate<uint8_t>(val);
}

void SmartBuffer::addEx16(uint16_t val)
{
	addExTemplate<uint16_t>(val);
}

void SmartBuffer::addEx32(uint32_t val)
{
	addExTemplate<uint32_t>(val);
}

void SmartBuffer::addEx64(uint64_t val)
{
	addExTemplate<uint64_t>(val);
}

void SmartBuffer::addEx(const void *src, size_t len)
{
	size_t requiredSize = m_index + len;
	ensureRemainingSize(requiredSize);
	add(src, len);
}

void SmartBuffer::incIndex(size_t size)
{
	m_index += size;
}

void SmartBuffer::setAt(size_t index, uint32_t val)
{
	*reinterpret_cast<uint32_t *>(&m_buf[index]) = val;
}

void SmartBuffer::printBuffer(void)
{
	static const int MSG_BUF_SIZE = 0x80;
	char buf[MSG_BUF_SIZE];
	snprintf(buf, MSG_BUF_SIZE, "m_index: %zd, m_buf: %p, m_size: %zd\n",
	        m_index, m_buf, m_size);
	string msg = buf;

	const int numValuePerLine = 16;
	size_t idx = 0;
	while (idx < m_index) {
		for (int col = 0;
		     col < numValuePerLine && idx < m_index; col++, idx++) {
			snprintf(buf, MSG_BUF_SIZE, "%02x ", m_buf[idx]);
			msg += buf;
		}
		msg += "\n";
	}
	MLPL_INFO(msg.c_str());
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
