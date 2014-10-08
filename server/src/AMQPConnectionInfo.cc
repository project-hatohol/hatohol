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

#include <string>
#include <amqp.h>
#include <Logger.h>
#include <StringUtils.h>
#include "AMQPConnectionInfo.h"

static const char  *DEFAULT_URL     = "amqp://localhost";
static const time_t DEFAULT_TIMEOUT = 1;

using namespace std;
using namespace hfl;

struct AMQPConnectionInfo::Impl {
	Impl()
	: m_URL(),
	  m_parsedURL(),
	  m_queueName(),
	  m_timeout(DEFAULT_TIMEOUT)
	{
		amqp_default_connection_info(&m_parsedURL);
		setURL(DEFAULT_URL);
	}

	~Impl()
	{
	}

	void setURL(const string &URL)
	{
		m_URL = normalizeURL(URL);
		int status = amqp_parse_url(const_cast<char *>(m_URL.c_str()),
					    &m_parsedURL);
		if (status != AMQP_STATUS_OK) {
			HFL_ERR("Bad broker URL: %s: <%s>\n",
				 amqp_error_string2(status),
				 m_URL.c_str());
		}
	}

	string m_URL;
	amqp_connection_info m_parsedURL;
	string m_queueName;
	time_t m_timeout;
	string m_tlsCertificatePath;
	string m_tlsKeyPath;
	string m_tlsCACertificatePath;
	bool m_isTLSVerifyEnabled;

private:
	string normalizeURL(const string &URL)
	{
		using StringUtils::hasPrefix;

		if (hasPrefix(URL, "amqp://"))
			return URL;
		if (hasPrefix(URL, "amqps://"))
			return URL;
		return string("amqp://") + URL;
	}
};

AMQPConnectionInfo::AMQPConnectionInfo()
: m_impl(new Impl())
{
}

AMQPConnectionInfo::~AMQPConnectionInfo()
{
}

const string &AMQPConnectionInfo::getURL(void) const
{
	return m_impl->m_URL;
}

void AMQPConnectionInfo::setURL(const string &URL)
{
	m_impl->setURL(URL);
}

const char *AMQPConnectionInfo::getHost(void) const
{
	return m_impl->m_parsedURL.host;
}

int AMQPConnectionInfo::getPort(void) const
{
	return m_impl->m_parsedURL.port;
}

const char *AMQPConnectionInfo::getUser(void) const
{
	return m_impl->m_parsedURL.user;
}

const char *AMQPConnectionInfo::getPassword(void) const
{
	return m_impl->m_parsedURL.password;
}

const char *AMQPConnectionInfo::getVirtualHost(void) const
{
	return m_impl->m_parsedURL.vhost;
}

bool AMQPConnectionInfo::useTLS(void) const
{
	return m_impl->m_parsedURL.ssl;
}

const string &AMQPConnectionInfo::getQueueName(void) const
{
	return m_impl->m_queueName;
}

void AMQPConnectionInfo::setQueueName(const string &queueName)
{
	m_impl->m_queueName = queueName;
}

time_t AMQPConnectionInfo::getTimeout(void) const
{
	return m_impl->m_timeout;
}

void AMQPConnectionInfo::setTimeout(const time_t &timeout)
{
	m_impl->m_timeout = timeout;
}

const string &AMQPConnectionInfo::getTLSCertificatePath(void) const
{
	return m_impl->m_tlsCertificatePath;
}

void AMQPConnectionInfo::setTLSCertificatePath(const string &path)
{
	m_impl->m_tlsCertificatePath = path;
}

const string &AMQPConnectionInfo::getTLSKeyPath(void) const
{
	return m_impl->m_tlsKeyPath;
}

void AMQPConnectionInfo::setTLSKeyPath(const string &path)
{
	m_impl->m_tlsKeyPath = path;
}

const string &AMQPConnectionInfo::getTLSCACertificatePath(void) const
{
	return m_impl->m_tlsCACertificatePath;
}

void AMQPConnectionInfo::setTLSCACertificatePath(const string &path)
{
	m_impl->m_tlsCACertificatePath = path;
}

bool AMQPConnectionInfo::isTLSVerifyEnabled(void) const
{
	return m_impl->m_isTLSVerifyEnabled;
}

void AMQPConnectionInfo::setTLSVerifyEnabled(const bool &enabled)
{
	m_impl->m_isTLSVerifyEnabled = enabled;
}

