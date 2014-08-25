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

#ifndef AMQPConnectionInfo_h
#define AMQPConnectionInfo_h

#include <string>
#include <memory>
#include "Params.h"

class AMQPConnectionInfo {
public:
	AMQPConnectionInfo(void);
	virtual ~AMQPConnectionInfo();

	const std::string &getURL(void) const;
	void setURL(const std::string &URL);

	const char *getHost(void) const;
	int getPort(void) const;
	const char *getUser(void) const;
	const char *getPassword(void) const;
	const char *getVirtualHost(void) const;
	bool useTLS(void) const;

	const std::string &getQueueName(void) const;
	void setQueueName(const std::string &queueName);

	time_t getTimeout(void) const;
	void setTimeout(const time_t &timeout);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // AMQPConnectionInfo_h
