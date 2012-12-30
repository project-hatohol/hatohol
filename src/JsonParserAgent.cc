#include <Logger.h>
using namespace mlpl;

#include "AsuraException.h"
#include "JsonParserAgent.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
JsonParserAgent::JsonParserAgent(const char *data)
: m_reader(NULL),
  m_error(NULL)
{
	m_error = NULL;
	m_parser = json_parser_new();
	if (!json_parser_load_from_data(m_parser, data, -1, &m_error))
		return;
	m_reader = json_reader_new(json_parser_get_root(m_parser));
}

JsonParserAgent::~JsonParserAgent()
{
	if (m_error)
		g_error_free(m_error);
	if (m_reader)
		g_object_unref(m_reader);
	if (m_parser)
		g_object_unref(m_parser);
}

const char *JsonParserAgent::getErrorMessage(void)
{
	if (!m_error)
		return "No error";
	return m_error->message;
}

bool JsonParserAgent::hasError(void)
{
	return m_error != NULL;
}

bool JsonParserAgent::read(const char *member, string &dest)
{
	internalCheck();
	if (!json_reader_read_member(m_reader, member))
		return false;
	dest = json_reader_get_string_value(m_reader);
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void JsonParserAgent::internalCheck(void)
{
	if (!m_reader)
		throw AsuraException("m_reader: NULL\n");

}
