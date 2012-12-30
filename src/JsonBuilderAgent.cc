#include "JsonBuilderAgent.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
JsonBuilderAgent::JsonBuilderAgent(void)
{
	m_builder = json_builder_new();
	json_builder_begin_object(m_builder);
}

JsonBuilderAgent::~JsonBuilderAgent()
{
	if (m_builder)
		g_object_unref(m_builder);
}

string JsonBuilderAgent::generate(void)
{
	JsonGenerator *generator = json_generator_new();
	JsonNode *root = json_builder_get_root(m_builder);
	json_generator_set_root(generator, root);

	gchar *str = json_generator_to_data(generator, NULL);
	string json_str = str;

	g_free(str);
	json_node_free(root);
	g_object_unref(generator);

	return json_str;
}

void JsonBuilderAgent::startObject(const char *member)
{
	if (member)
		json_builder_set_member_name(m_builder, member);
	json_builder_begin_object(m_builder);
}

void JsonBuilderAgent::endObject(void)
{
	json_builder_end_object(m_builder);
}

void JsonBuilderAgent::addNull(const char *member)
{
	json_builder_set_member_name(m_builder, member);
	json_builder_add_null_value(m_builder);
}

void JsonBuilderAgent::add(const char *member, const char *value)
{
	json_builder_set_member_name(m_builder, member);
	json_builder_add_string_value(m_builder, value);
}

void JsonBuilderAgent::add(const char *member, gint64 value)
{
	json_builder_set_member_name(m_builder, member);
	json_builder_add_int_value(m_builder, value);
}
