#ifndef JsonParserAgent_h
#define JsonParserAgent_h

#include <string>
using namespace std;

#include <glib.h>
#include <json-glib/json-glib.h>

class JsonParserAgent
{
public:
	JsonParserAgent(const char *data);
	virtual ~JsonParserAgent();
	const char *getErrorMessage(void);
	bool hasError(void);
	bool read(const char *member, string &dest);

protected:
	void internalCheck(void);

private:
	JsonParser *m_parser;
	JsonReader *m_reader;
	GError *m_error;
};

#endif // JsonParserAgent_h
