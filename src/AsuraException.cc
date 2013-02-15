#include "AsuraException.h"

AsuraException::AsuraException(const string &brief)
: m_what(brief)
{
}

AsuraException::~AsuraException() _GLIBCXX_USE_NOEXCEPT
{
}

const char* AsuraException::what() const _GLIBCXX_USE_NOEXCEPT
{
	return m_what.c_str();
}
