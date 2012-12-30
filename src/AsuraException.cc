#include <stdarg.h>
#include <glib.h>
#include "AsuraException.h"

AsuraException::AsuraException(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	gchar *msg = g_strdup_vprintf(fmt, ap);
	m_message = msg;
	g_free(msg);
	va_end(ap);
}

AsuraException::~AsuraException()
{
}

const char *AsuraException::getMessage(void)
{
	return m_message.c_str();
}
