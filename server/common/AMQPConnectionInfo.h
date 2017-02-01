/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <string>
#include <memory>
#include "Params.h"

class AMQPConnectionInfo {
public:
	AMQPConnectionInfo(void);
	AMQPConnectionInfo(const AMQPConnectionInfo &info);
	AMQPConnectionInfo &operator=(const AMQPConnectionInfo &info);
	virtual ~AMQPConnectionInfo();

	const std::string &getURL(void) const;
	void setURL(const std::string &URL);

	const char *getHost(void) const;
	int getPort(void) const;
	const char *getUser(void) const;
	const char *getPassword(void) const;
	const char *getVirtualHost(void) const;
	bool useTLS(void) const;

	const std::string &getConsumerQueueName(void) const;
	void setConsumerQueueName(const std::string &queueName);
	const std::string &getPublisherQueueName(void) const;
	void setPublisherQueueName(const std::string &queueName);

	time_t getTimeout(void) const;
	void setTimeout(const time_t &timeout);

	const std::string &getTLSCertificatePath(void) const;
	void setTLSCertificatePath(const std::string &path);

	const std::string &getTLSKeyPath(void) const;
	void setTLSKeyPath(const std::string &path);

	const std::string &getTLSCACertificatePath(void) const;
	void setTLSCACertificatePath(const std::string &path);

	bool isTLSVerifyEnabled(void) const;
	void setTLSVerifyEnabled(const bool &enabled);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

