#include "SQLProcessorException.h"

SQLProcessorException::SQLProcessorException(const string &brief)
: AsuraException(brief)
{
}

SQLProcessorException::~SQLProcessorException() _GLIBCXX_USE_NOEXCEPT
{
}
