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

#ifndef AMQPConsumer_h
#define AMQPConsumer_h

#include "HatoholThreadBase.h"

class AMQPConnectionInfo;
class AMQPMessageHandler;

class AMQPConsumer : public HatoholThreadBase {
public:
	AMQPConsumer(const AMQPConnectionInfo &connectionInfo,
		     AMQPMessageHandler *handler);
	virtual ~AMQPConsumer();

protected:
	virtual gpointer mainThread(HatoholThreadArg *arg) override;

private:
	const AMQPConnectionInfo &m_connectionInfo;
	AMQPMessageHandler *m_handler;
};

#endif // AMQPConsumer_h
