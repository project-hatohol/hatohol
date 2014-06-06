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

#include <cstdio>
#include <SimpleSemaphore.h>
#include "HatoholArmPluginBase.h"

using namespace mlpl;

struct HatoholArmPluginBase::PrivateContext {
	SimpleSemaphore replyWaitSem;
	SmartBuffer     responseBuf;

	PrivateContext(void)
	: replyWaitSem(0)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginBase::HatoholArmPluginBase(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

HatoholArmPluginBase::~HatoholArmPluginBase()
{
	if (m_ctx)
		delete m_ctx;
}

SmartTime HatoholArmPluginBase::getTimestampOfLastTrigger(void)
{
	SmartBuffer cmdBuf(sizeof(HapiCommandHeader));
	HapiCommandHeader *cmdHeader = cmdBuf.getPointer<HapiCommandHeader>();
	cmdHeader->type = HAPI_MSG_COMMAND;
	cmdHeader->code = HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER;
	send(cmdBuf);
	waitResponseAndCheckHeader();

	const HapiResTimestampOfLastTrigger *body = 
	  getResponseBody<HapiResTimestampOfLastTrigger>(m_ctx->responseBuf);
	const timespec ts = {(time_t)body->timestamp, body->nanosec};
	return SmartTime(ts);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HatoholArmPluginBase::onGotResponse(
  const HapiResponseHeader *header, SmartBuffer &resBuf)
{
	resBuf.handOver(m_ctx->responseBuf);
	m_ctx->replyWaitSem.post();
}

void HatoholArmPluginBase::waitResponseAndCheckHeader(void)
{
	m_ctx->replyWaitSem.wait();

	// To check the sainity of the header
	getResponseHeader(m_ctx->responseBuf);
}
