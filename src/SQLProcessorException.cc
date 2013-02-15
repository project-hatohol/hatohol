#include "SQLProcessorException.h"

SQLProcessorException::SQLProcessorException(const string &brief)
: m_what(brief)
{
}

SQLProcessorException::~SQLProcessorException() _GLIBCXX_USE_NOEXCEPT
{
}

const char* SQLProcessorException::what() const _GLIBCXX_USE_NOEXCEPT
{
	return m_what.c_str();
}
