/*
 * Copyright (C) 2014-2015 Project Hatohol
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
#include <functional>
#include "HatoholThreadBase.h"
#include "AMQPConnection.h"

class AMQPMessageHandler;

class AMQPConsumer : public HatoholThreadBase {
public:
	enum ConnectionStatus {
		CONN_INIT,
		CONN_ESTABLISHED,
		CONN_DISCONNECTED,
	};
	typedef std::function<void(const ConnectionStatus &)>
	  ConnectionChangeCallback;

	AMQPConsumer(const AMQPConnectionInfo &connectionInfo,
		     AMQPMessageHandler *handle);
	AMQPConsumer(std::shared_ptr<AMQPConnection> connection,
		     AMQPMessageHandler *handler);
	virtual ~AMQPConsumer();

	std::shared_ptr<AMQPConnection> getConnection(void);
	void setConnectionChangeCallback(ConnectionChangeCallback cb);

protected:
	virtual gpointer mainThread(HatoholThreadArg *arg) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

