#ifndef AsuraException_h
#define AsuraException_h

#include <Logger.h>
using namespace mlpl;

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

#define THROW_ASURA_EXCEPTION(FMT, ...) \
do { \
	string msg = StringUtils::sprintf(FMT, ##__VA_ARGS__); \
	throw new AsuraException(msg); \
} while (0)

#define THROW_ASURA_EXCEPTION_WITH_LOG(LOG_LV, FMT, ...) \
do { \
	MLPL_##LOG_LV(FMT, ##__VA_ARGS__); \
	THROW_ASURA_EXCEPTION(FMT, ##__VA_ARGS__); \
} while (0)

#endif // AsuraException_h
