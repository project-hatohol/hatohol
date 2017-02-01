/*
 * Copyright (C) 2013-2015 Project Hatohol
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
#include <cstdio>
#include <string>
#include "ResidentProtocol.h"
#include "NamedPipe.h"
#include "HatoholException.h"
#include "DBTablesAction.h"

class ResidentCommunicator {
public:
	ResidentCommunicator(void);
	virtual ~ResidentCommunicator();

	/**
	 * Get the body size.
	 *
	 * After calling this function, the index of sbuf is changed.
	 */
	static size_t getBodySize(mlpl::SmartBuffer &sbuf);

	/**
	 * Get the packet type.
	 *
	 * After calling this function, the index of sbuf is changed.
	 */
	static int getPacketType(mlpl::SmartBuffer &sbuf);

	mlpl::SmartBuffer &getBuffer(void);
	void setHeader(uint32_t bodySize, uint16_t type);
	void push(NamedPipe &namedPipe);
	void addModulePath(const std::string &modulePath);
	void addModuleOption(const std::string &moduleOption);
	void setModuleLoaded(uint32_t code);
	void setNotifyEventBody(const ActionIdType &actionId,
	                        const EventInfo &eventInfo,
	                        const std::string &sessionId);
	void setNotifyEventAck(uint32_t resuletCode);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

template<typename ArgType>
class ResidentPullHelper {

	typedef void (*PullCBType)
	  (GIOStatus stat, mlpl::SmartBuffer &sbuf, size_t size, ArgType *ctx);

public:
	ResidentPullHelper(void)
	: m_pullCallback(NULL),
	  m_pullCallbackArg(NULL),
	  m_pipe(NULL)
	{
	}

	virtual ~ResidentPullHelper()
	{
	}

	static void pullCallbackGate
	  (GIOStatus stat, mlpl::SmartBuffer &sbuf, size_t size, void *_this)
	{
		ResidentPullHelper<ArgType> *obj =
		  static_cast<ResidentPullHelper<ArgType> *>(_this);
		PullCBType cbFunc = obj->m_pullCallback;
		HATOHOL_ASSERT(cbFunc, "pullCallback is NULL.");

		// To avoid the assertion from being called when 
		// pullHeader() is called in the following callback.
		obj->m_pullCallback = NULL;
		(*cbFunc)(stat, sbuf, size, obj->m_pullCallbackArg);
	}

	void pullData(uint32_t size, PullCBType cbFunc)
	{
		HATOHOL_ASSERT(!m_pullCallback,
		   "The previous pull callback may be still alive.");
		HATOHOL_ASSERT(m_pipe, "m_pipe is NULL.");
		m_pullCallback = cbFunc;
		m_pipe->pull(size, pullCallbackGate, this);
	}

	void pullHeader(PullCBType cbFunc)
	{
		pullData(RESIDENT_PROTO_HEADER_LEN, cbFunc);
	}

	void setPullCallbackArg(ArgType *arg)
	{
		m_pullCallbackArg = arg;
	}

protected:
	void initResidentPullHelper(NamedPipe *pipe, ArgType *arg)
	{
		m_pipe = pipe;
		m_pullCallbackArg = arg;
	}

private:
	PullCBType  m_pullCallback;
	ArgType    *m_pullCallbackArg;
	NamedPipe  *m_pipe;
};

