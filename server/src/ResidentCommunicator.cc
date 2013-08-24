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

void ResidentCommunicator::setHeader(uint32_t bodySize, uint16_t type)
{
	size_t bufSize = RESIDENT_PROTO_HEADER_LEN + bodySize;
	m_ctx->sbuf.ensureRemainingSize(bufSize);
	m_ctx->sbuf.resetIndex();
	m_ctx->sbuf.add32(bodySize);
	m_ctx->sbuf.add16(type);
}

void ResidentCommunicator::push(NamedPipe &namedPipe)
{
	namedPipe.push(m_ctx->sbuf);
}
