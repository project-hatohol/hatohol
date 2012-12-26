#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "smart-buffer.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
smart_buffer::smart_buffer(void)
: m_index(0),
  m_buf(NULL),
  m_size(0)
{
}

smart_buffer::~smart_buffer()
{
	if (m_buf)
		free(m_buf);
}

smart_buffer::operator const char *() const
{
	return reinterpret_cast<const char *>(m_buf);
}

size_t smart_buffer::size(void) const
{
	return m_size;
}

void smart_buffer::alloc(size_t size)
{
	m_buf = static_cast<uint8_t *>(realloc(m_buf, size));
	if (m_buf == NULL)
		throw smart_buffer_exception();
	m_index = 0;
	m_size = size;
}

void smart_buffer::add8(uint8_t val)
{
	m_buf[m_index] = val;
	m_index++;
}

void smart_buffer::add16(uint16_t val)
{
	*reinterpret_cast<uint16_t *>(&m_buf[m_index]) = val;
	m_index += 2;
}

void smart_buffer::add32(uint32_t val)
{
	*reinterpret_cast<uint32_t *>(&m_buf[m_index]) = val;
	m_index += 4;
}

void smart_buffer::add(const void *src, size_t len)
{
	memcpy(&m_buf[m_index], src, len);
	m_index += len;
}

void smart_buffer::add_zero(size_t len)
{
	memset(&m_buf[m_index], 0, len);
	m_index += len;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
