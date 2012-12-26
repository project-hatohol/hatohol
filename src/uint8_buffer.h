#ifndef uint8_buffer_h
#define uint8_buffer_h

#include <vector>
using namespace std;

#include <stdint.h>

class uint8_buffer {
	vector<uint8_t> m_buf;
public:
	operator const char *() const;
	size_t size(void) const;
};

#endif // uint8_buffer_h

