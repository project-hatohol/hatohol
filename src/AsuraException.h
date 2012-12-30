#ifndef AsuraException_h
#define AsuraExceptiont_h

#include <string>
using namespace std;

class AsuraException {
public:
	AsuraException(const char *fmt, ...);
	virtual ~AsuraException();
	const char *getMessage();

private:
	string m_message;
};

#endif // AsuraExceptiont_h

