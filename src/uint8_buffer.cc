#include "uint8_buffer.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
uint8_buffer::operator const char *() const
{
	return reinterpret_cast<const char *>(&m_buf[0]);
}

size_t uint8_buffer::size(void) const
{
	return m_buf.size();
}
// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
