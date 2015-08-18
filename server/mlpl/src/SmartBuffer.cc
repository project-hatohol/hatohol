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

#include <cstdio>
#include <cstring>
#include <string>
#include <inttypes.h>
using namespace std;

#include "StringUtils.h"
#include "SmartBuffer.h"
#include "Logger.h"
using namespace mlpl;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SmartBuffer::SmartBuffer(void)
: m_index(0),
  m_buf(NULL),
  m_size(0),
  m_watermark(0)
{
}

SmartBuffer::SmartBuffer(size_t size)
: m_index(0),
  m_buf(NULL),
  m_size(0),
  m_watermark(0)
{
	alloc(size);
}

SmartBuffer::SmartBuffer(const SmartBuffer &rhs)
: m_index(rhs.m_index),
  m_buf(NULL),
  m_size(rhs.m_size),
  m_watermark(rhs.m_watermark)
{
	alloc(m_size);
	memcpy(m_buf, rhs.m_buf, m_size);
}

const SmartBuffer &SmartBuffer::operator=(const SmartBuffer &rhs)
{
	if (this == &rhs)
		return *this;

	m_index = rhs.m_index;
	m_buf = NULL;
	m_size = rhs.m_size;
	m_watermark = rhs.m_watermark;
	alloc(m_size);
	memcpy(m_buf, rhs.m_buf, m_size);
	return *this;
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

size_t SmartBuffer::watermark(void) const
{
	return m_watermark;
}

void SmartBuffer::alloc(size_t size, bool _resetIndexDeep)
{
	uint8_t *newBuffer = static_cast<uint8_t *>(realloc(m_buf, size));
	if (newBuffer == NULL)
		throw SmartBufferException();
	m_buf = newBuffer;
	if (_resetIndexDeep)
		resetIndexDeep();
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

void SmartBuffer::setIndex(const size_t &index)
{
	m_index = index;
}

void SmartBuffer::resetIndexDeep(void)
{
	resetIndex();
	m_watermark = 0;
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
	incIndex(len);
}

void SmartBuffer::addZero(size_t size)
{
	memset(&m_buf[m_index], 0, size);
	incIndex(size);
}

size_t SmartBuffer::insertString(const string &str, const size_t &bodyIndex)
{
	const size_t length = str.size();
	if (length > UINT32_MAX) {
		const string msg = StringUtils::sprintf(
		  "Too long string length: %zd", length);
		throw std::range_error(msg);
	}
	const size_t sizeNullTerm = 1;
	const size_t nextBodyIndex = bodyIndex + length + sizeNullTerm;
	if (nextBodyIndex > m_size) {
		const string msg = StringUtils::sprintf(
		  "Invalid paramter: string length: %zd, "
		  "body index: %zd, buff size: %zd",
		  length, bodyIndex, m_size);
		throw std::out_of_range(msg);
	}

	const ssize_t offset = bodyIndex - m_index;
	if (offset > INT32_MAX || offset < INT32_MIN) {
		const string msg = StringUtils::sprintf(
		  "offset overflow: string length: %zd, "
		  "body index: %zd, buff index: %zd",
		  length, bodyIndex, m_index);
		throw std::out_of_range(msg);
	}

	StringHeader header;
	header.size = length;
	header.offset = offset;
	add(&header, sizeof(header));
	memcpy(m_buf + bodyIndex, str.c_str(), length + sizeNullTerm);

	setWatermarkIfNeeded(nextBodyIndex);
	return nextBodyIndex;
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
	setWatermarkIfNeeded();
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
	MLPL_INFO("%s", msg.c_str());
}

std::string SmartBuffer::extractString(const StringHeader *header)
{
	const char *body =
	  reinterpret_cast<const char *>(header) + header->offset;
	return string(body, header->size);
}

std::string SmartBuffer::extractStringAndIncIndex(void)
{
	const StringHeader *header = getPointerAndIncIndex<StringHeader>();
	return extractString(header);
}

SmartBuffer *SmartBuffer::takeOver(void)
{
	SmartBuffer *sbuf = new SmartBuffer();
	handOver(*sbuf);
	return sbuf;
}

void SmartBuffer::handOver(SmartBuffer &sbuf)
{
	if (sbuf.m_buf)
		free(sbuf.m_buf);
	sbuf.m_index = m_index;
	sbuf.m_buf = m_buf;
	sbuf.m_size = m_size;
	sbuf.m_watermark = m_watermark;
	resetIndexDeep();
	m_buf = NULL;
	m_size = 0;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void SmartBuffer::setWatermarkIfNeeded(const size_t &index)
{
	if (index > m_watermark)
		m_watermark = index;
}
