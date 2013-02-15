#ifndef AsuraException_h
#define AsuraException_h

#include <exception>
#include <string>
using namespace std;

class AsuraException : public exception
{
public:
	explicit AsuraException(const string &brief);
	virtual ~AsuraException() _GLIBCXX_USE_NOEXCEPT;
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT;
private:
	string m_what;
};

#endif // AsuraException_h
