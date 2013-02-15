#ifndef SQLProcessorException_h
#define SQLProcessorException_h

#include <exception>
#include <string>
using namespace std;

class SQLProcessorException : public exception
{
public:
	explicit SQLProcessorException(const string &brief);
	virtual ~SQLProcessorException() _GLIBCXX_USE_NOEXCEPT;
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT;
private:
	string m_what;
};

#endif // SQLProcessorException_h
