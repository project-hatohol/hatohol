/*
 * Copyright (C) 2013 Project Hatohol
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

#ifndef ResidentCommunicator_h
#define ResidentCommunicator_h

#include <cstdio>
#include <string>

#include "ResidentProtocol.h"
#include "NamedPipe.h"
#include "HatoholException.h"
#include "DBClientAction.h"

class ResidentCommunicator {
public:
	ResidentCommunicator(void);
	virtual ~ResidentCommunicator();

	/**
	 * Get the packet type.
	 *
	 * After calling this function, the index of sbuf is changed.
	 */
	static int getPacketType(SmartBuffer &sbuf);

	void setHeader(uint32_t bodySize, uint16_t type);
	void push(NamedPipe &namedPipe);
	void addModulePath(const std::string &modulePath);
	void addModuleOption(const std::string &moduleOption);
	void setNotifyEventBody(int actionId, const EventInfo &eventInfo);
	void setNotifyEventAck(uint32_t resuletCode);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

template<typename ArgType>
class ResidentPullHelper {

	typedef void (*PullCBType)
	  (GIOStatus stat, SmartBuffer &sbuf, size_t size, ArgType *ctx);

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
	  (GIOStatus stat, SmartBuffer &sbuf, size_t size, void *_this)
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

#endif // ResidentCommunicator_h
