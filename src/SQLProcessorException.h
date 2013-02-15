#ifndef SQLProcessorException_h
#define SQLProcessorException_h

#include "AsuraException.h"

class SQLProcessorException : public AsuraException
{
public:
	explicit SQLProcessorException(const string &brief);
	virtual ~SQLProcessorException() _GLIBCXX_USE_NOEXCEPT;
private:
};

#endif // SQLProcessorException_h
