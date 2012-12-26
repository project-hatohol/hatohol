#ifndef smart_buffer_h
#define smart_buffer_h

#include <vector>
using namespace std;

#include <stdint.h>

class smart_buffer_exception {
};

class smart_buffer {
	size_t   m_index;
	uint8_t *m_buf;
	size_t   m_size;
public:
	smart_buffer(void);
	virtual ~smart_buffer();
	operator const char *() const;
	size_t size(void) const;
	void alloc(size_t size);
	void add8(uint8_t val);
	void add16(uint16_t val);
	void add32(uint32_t val);
	void add(const void *src, size_t len);
};

#endif // smart_buffer_h

