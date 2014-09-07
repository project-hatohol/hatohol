/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <libsoup/soup.h>
#include <SimpleSemaphore.h>
#include <Mutex.h>
#include <Reaper.h>
#include <stack>
#include "Utils.h"
#include "HapProcessStandard.h"
#include "JSONBuilder.h"
#include "JSONParser.h"

using namespace std;
using namespace mlpl;

static const char *MIME_JSON = "application/json";

struct OpenStackEndPoint {
	string publicURL;
};

// ---------------------------------------------------------------------------
// Class: HapProcessCeilometer
// ---------------------------------------------------------------------------
class HapProcessCeilometer : public HapProcessStandard {
public:
	HapProcessCeilometer(int argc, char *argv[]);

protected:
	void updateAuthTokenIfNeeded(void);
	bool startObject(JSONParser &parser, const string &name);
	bool read(JSONParser &parser, const string &member, string &dest);
	bool parseReplyToknes(SoupMessage *msg);
	bool parserEndpoints(JSONParser &parser, const unsigned int &index);

	virtual void acquireData(void) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

struct HapProcessCeilometer::Impl {
	string osUsername;
	string osPassword;
	string osTenantName;
	string osAuthURL;

	string token;
	OpenStackEndPoint ceilometerEP;

	Impl(void)
	{
		// Temporary paramters
		osUsername   = "admin";
		osPassword   = "admin";
		osTenantName = "admin";
		osAuthURL    = "http://botctl:35357/v2.0";
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapProcessCeilometer::HapProcessCeilometer(int argc, char *argv[])
: HapProcessStandard(argc, argv),
  m_impl(new Impl())
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HapProcessCeilometer::updateAuthTokenIfNeeded(void)
{
	if (!m_impl->token.empty())
		return;

	string url = m_impl->osAuthURL;
	url += "/tokens";
	SoupMessage *msg = soup_message_new(SOUP_METHOD_POST, url.c_str());
	Reaper<void> msgReaper(msg, g_object_unref);
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	JSONBuilder builder;
	builder.startObject();
	builder.startObject("auth");
	builder.add("tenantName", m_impl->osTenantName);
	builder.startObject("passwordCredentials");
	builder.add("username", m_impl->osUsername);
	builder.add("password", m_impl->osPassword);
	builder.endObject(); // passwordCredentials
	builder.endObject(); // auth
	builder.endObject();

	string request_body = builder.generate();
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	SoupSession *session = soup_session_new();
	guint ret = soup_session_send_message(session, msg);
	if (ret != SOUP_STATUS_OK) {
		MLPL_ERR("Failed to connect: %d, URL: %s\n", ret, url.c_str());
		return;
	}
	if (!parseReplyToknes(msg)) {
		MLPL_DBG("body: %" G_GOFFSET_FORMAT ", %s\n",
		         msg->response_body->length, msg->response_body->data);
		return;
	}
}

bool HapProcessCeilometer::parseReplyToknes(SoupMessage *msg)
{
	JSONParser parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser %s\n", parser.getErrorMessage());
		return false;
	}

	if (!startObject(parser, "access"))
		return false;
	if (!startObject(parser, "token"))
		return false;

	if (!read(parser, "id", m_impl->token))
		return false;

	parser.endObject(); // access
	if (!startObject(parser, "serviceCatalog"))
		return false;

	const unsigned int count = parser.countElements();
	for (unsigned int idx = 0; idx < count; idx++) {
		if (!parserEndpoints(parser, idx))
			return false;
		if (!m_impl->ceilometerEP.publicURL.empty())
			break;
	}

	// check if there's information about the endpoint of ceilometer
	if (m_impl->ceilometerEP.publicURL.empty()) {
		MLPL_ERR("Failed to get an endpoint of ceilometer\n");
		return false;
	}
	return true;
}

bool HapProcessCeilometer::parserEndpoints(JSONParser &parser,
                                    const unsigned int &index)
{
	JSONParser::PositionStack parserRewinder(parser);
	if (! parserRewinder.pushElement(index)) {
		MLPL_ERR("Failed to parse an element, index: %u\n", index);
		return false;
	}

	string name;
	if (!read(parser, "name", name))
		return false;
	if (name != "ceilometer")
		return true;

	if (!parserRewinder.pushObject("endpoints"))
		return false;

	const unsigned int count = parser.countElements();
	for (unsigned int i = 0; i < count; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse an element, index: %u\n", i);
			return false;
		}
		bool succeeded = read(parser, "publicURL",
		                      m_impl->ceilometerEP.publicURL);
		parser.endElement();
		if (!succeeded)
			return false;

		// NOTE: Currently we only use the first information even if
		// there're multiple endpoints
		break;
	}

	return true;
}

bool HapProcessCeilometer::startObject(
  JSONParser &parser, const string &name)
{
	if (parser.startObject(name))
		return true;
	MLPL_ERR("Not found object: %s\n", name.c_str());
	return false;
}

bool HapProcessCeilometer::read(
  JSONParser &parser, const string &member, string &dest)
{
	if (parser.read(member, dest))
		return true;
	MLPL_ERR("Failed to read: %s\n", member.c_str());
	return false;
}

void HapProcessCeilometer::acquireData(void)
{
	updateAuthTokenIfNeeded();
	// TODO: Add get trigger, event, and so on.
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	MLPL_INFO("hatohol-arm-plugin-ceilometer. ver: %s\n", PACKAGE_VERSION);
	HapProcessCeilometer hapProc(argc, argv);
	return hapProc.mainLoopRun();
}
