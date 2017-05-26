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

#include <cstring>
#include <string>
using namespace std;

#include "SmartBuffer.h"
using namespace mlpl;

#include "ResidentCommunicator.h"

struct ResidentCommunicator::Impl {
	SmartBuffer sbuf;
};

static_assert(sizeof(mlpl::SmartBuffer::StringHeader) == sizeof(ResidentStringHeader),
	"check ResidentStringHeader in ResidentProtocol.h");

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ResidentCommunicator::ResidentCommunicator(void)
: m_impl(new Impl())
{
}

ResidentCommunicator::~ResidentCommunicator()
{
}

size_t ResidentCommunicator::getBodySize(SmartBuffer &sbuf)
{
	HATOHOL_ASSERT(sbuf.size() >= RESIDENT_PROTO_HEADER_LEN,
	               "Too small: sbuf.size(): %zd\n", sbuf.size());
	sbuf.resetIndex();
	return *sbuf.getPointer<uint32_t>();
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
	return m_impl->sbuf;
}

void ResidentCommunicator::setHeader(uint32_t bodySize, uint16_t type)
{
	size_t bufSize = RESIDENT_PROTO_HEADER_LEN + bodySize;
	m_impl->sbuf.ensureRemainingSize(bufSize);
	m_impl->sbuf.resetIndexDeep();
	m_impl->sbuf.add32(bodySize);
	m_impl->sbuf.add16(type);
}

void ResidentCommunicator::push(NamedPipe &namedPipe)
{
	namedPipe.push(m_impl->sbuf);
}

void ResidentCommunicator::addModulePath(const string &modulePath)
{
	size_t len = modulePath.size();
	m_impl->sbuf.add16(len);
	memcpy(m_impl->sbuf.getPointer<void>(), modulePath.c_str(), len);
	m_impl->sbuf.incIndex(len);
}

void ResidentCommunicator::addModuleOption(const std::string &moduleOption)
{
	size_t len = moduleOption.size();
	m_impl->sbuf.add16(len);
	memcpy(m_impl->sbuf.getPointer<void>(), moduleOption.c_str(), len);
	m_impl->sbuf.incIndex(len);
}

void ResidentCommunicator::setModuleLoaded(uint32_t code)
{
	setHeader(RESIDENT_PROTO_MODULE_LOADED_CODE_LEN,
	          RESIDENT_PROTO_PKT_TYPE_MODULE_LOADED);
	m_impl->sbuf.add32(code);
}

void ResidentCommunicator::setNotifyEventBody(
  const ActionIdType &actionId, const EventInfo &eventInfo,
  const string &sessionId)
{
	const size_t lenNullTerm = 1;
	const uint32_t bodySize =
	  RESIDENT_PROTO_EVENT_BODY_BASE_LEN +
	  eventInfo.hostIdInServer.size() + lenNullTerm +
	  eventInfo.id.size()             + lenNullTerm +
	  eventInfo.triggerId.size()      + lenNullTerm;
	size_t bodyIdx =
	  RESIDENT_PROTO_HEADER_LEN + RESIDENT_PROTO_EVENT_BODY_BASE_LEN;
	setHeader(bodySize,
	          RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT);
	m_impl->sbuf.add32(actionId);
	m_impl->sbuf.add32(eventInfo.serverId);
	bodyIdx = m_impl->sbuf.insertString(eventInfo.hostIdInServer, bodyIdx);
	m_impl->sbuf.add64(eventInfo.time.tv_sec);
	m_impl->sbuf.add32(eventInfo.time.tv_nsec);
	bodyIdx = m_impl->sbuf.insertString(eventInfo.id, bodyIdx);
	m_impl->sbuf.add16(eventInfo.type);
	bodyIdx = m_impl->sbuf.insertString(eventInfo.triggerId, bodyIdx);
	m_impl->sbuf.add16(eventInfo.status);
	m_impl->sbuf.add16(eventInfo.severity);
	m_impl->sbuf.add(sessionId.c_str(), HATOHOL_SESSION_ID_LEN);
}

void ResidentCommunicator::setNotifyEventAck(uint32_t resultCode)
{
	setHeader(RESIDENT_PROTO_EVENT_ACK_CODE_LEN,
	          RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT_ACK);
	m_impl->sbuf.add32(resultCode);
}
