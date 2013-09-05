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
 
#include <cstring>
#include <string>
using namespace std;

#include "SmartBuffer.h"
using namespace mlpl;

#include "ResidentCommunicator.h"

struct ResidentCommunicator::PrivateContext {
	SmartBuffer sbuf;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ResidentCommunicator::ResidentCommunicator(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ResidentCommunicator::~ResidentCommunicator()
{
	if (m_ctx)
		delete m_ctx;
}

int ResidentCommunicator::getPacketType(SmartBuffer &sbuf)
{
	HATOHOL_ASSERT(sbuf.size() >= RESIDENT_PROTO_HEADER_LEN,
	               "Too small: sbuf.size(): %zd\n", sbuf.size());
	sbuf.resetIndex();
	sbuf.incIndex(RESIDENT_PROTO_HEADER_PKT_SIZE_LEN);
	return *sbuf.getPointer<uint16_t>();
}

SmartBuffer &ResidentCommunicator::getBuffer(void)
{
	return m_ctx->sbuf;
}

void ResidentCommunicator::setHeader(uint32_t bodySize, uint16_t type)
{
	size_t bufSize = RESIDENT_PROTO_HEADER_LEN + bodySize;
	m_ctx->sbuf.ensureRemainingSize(bufSize);
	m_ctx->sbuf.resetIndexDeep();
	m_ctx->sbuf.add32(bodySize);
	m_ctx->sbuf.add16(type);
}

void ResidentCommunicator::push(NamedPipe &namedPipe)
{
	namedPipe.push(m_ctx->sbuf);
}

void ResidentCommunicator::addModulePath(const string &modulePath)
{
	size_t len = modulePath.size();
	m_ctx->sbuf.add16(len);
	memcpy(m_ctx->sbuf.getPointer<void>(), modulePath.c_str(), len);
	m_ctx->sbuf.incIndex(len);
}

void ResidentCommunicator::addModuleOption(const std::string &moduleOption)
{
	size_t len = moduleOption.size();
	m_ctx->sbuf.add16(len);
	memcpy(m_ctx->sbuf.getPointer<void>(), moduleOption.c_str(), len);
	m_ctx->sbuf.incIndex(len);
}

void ResidentCommunicator::setModuleLoaded(uint32_t code)
{
	setHeader(RESIDENT_PROTO_MODULE_LOADED_CODE_LEN,
	          RESIDENT_PROTO_PKT_TYPE_MODULE_LOADED);
	m_ctx->sbuf.add32(code);
}

void ResidentCommunicator::setNotifyEventBody(
  int actionId, const EventInfo &eventInfo)
{
	setHeader(RESIDENT_PROTO_EVENT_BODY_LEN,
	          RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT);
	m_ctx->sbuf.add32(actionId);
	m_ctx->sbuf.add32(eventInfo.serverId);
	m_ctx->sbuf.add64(eventInfo.hostId);
	m_ctx->sbuf.add64(eventInfo.time.tv_sec);
	m_ctx->sbuf.add32(eventInfo.time.tv_nsec);
	m_ctx->sbuf.add64(eventInfo.id); // Event ID
	m_ctx->sbuf.add16(eventInfo.type);
	m_ctx->sbuf.add64(eventInfo.triggerId);
	m_ctx->sbuf.add16(eventInfo.status);
	m_ctx->sbuf.add16(eventInfo.severity);
}

void ResidentCommunicator::setNotifyEventAck(uint32_t resultCode)
{
	setHeader(RESIDENT_PROTO_EVENT_ACK_CODE_LEN,
	          RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT_ACK);
	m_ctx->sbuf.add32(resultCode);
}
