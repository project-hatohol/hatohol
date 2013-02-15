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

#define THROW_SQL_PROCESSOR_EXCEPTION(FMT, ...) \
do { \
	string msg = StringUtils::sprintf(FMT, ##__VA_ARGS__); \
	throw new SQLProcessorException(msg); \
} while (0)

#endif // SQLProcessorException_h
