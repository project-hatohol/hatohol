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
#include <ReadWriteLock.h>
#include "ArmStatus.h"

using namespace mlpl;

// ---------------------------------------------------------------------------
// ArmInfo
// ---------------------------------------------------------------------------
ArmInfo::ArmInfo(void)
: running(false),
  stat(ARM_WORK_STAT_INIT),
  numTryToGet(0),
  numFailure(0)
{
}

// ---------------------------------------------------------------------------
// Private context
// ---------------------------------------------------------------------------
struct ArmStatus::PrivateContext {
	ReadWriteLock rwlock;
	ArmInfo       armInfo;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmStatus::ArmStatus(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ArmStatus::~ArmStatus()
{
	if (m_ctx)
		delete m_ctx;
}

ArmInfo ArmStatus::getArmInfo(void) const
{
	// This method returns a copy for MT-safe.
	ArmInfo armInfo;
	m_ctx->rwlock.readLock();
	armInfo = m_ctx->armInfo;
	m_ctx->rwlock.unlock();
	return armInfo;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
