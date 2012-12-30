#ifndef JsonBuilderAgent_h
#define JsonBuilderAgent_h

#include <string>
using namespace std;

#include <json-glib/json-glib.h>

class JsonBuilderAgent
{
public:
	JsonBuilderAgent(void);
	~JsonBuilderAgent();
	string generate(void);
	void startObject(const char *member = NULL);
	void endObject(void);
	void add(const char *member, const char *value);
	void add(const char *member, gint64 value);
	void addNull(const char *member);

private:
	JsonBuilder *m_builder;
};

#endif // JsonBuilderAgent_h
