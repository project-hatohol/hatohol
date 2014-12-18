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

#include <cstdio>
#include <ReadWriteLock.h>
#include <Logger.h>
#include "ArmStatus.h"

using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// ArmInfo
// ---------------------------------------------------------------------------
ArmInfo::ArmInfo(void)
: running(false),
  stat(ARM_WORK_STAT_INIT),
  numUpdate(0),
  numFailure(0)
{
}

// ---------------------------------------------------------------------------
// Private context
// ---------------------------------------------------------------------------
struct ArmStatus::Impl {
	ReadWriteLock rwlock;
	ArmInfo       armInfo;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmStatus::ArmStatus(void)
: m_impl(new Impl())
{
}

ArmStatus::~ArmStatus()
{
}

ArmInfo ArmStatus::getArmInfo(void) const
{
	// This method returns a copy for MT-safe.
	ArmInfo armInfo;
	m_impl->rwlock.readLock();
	armInfo = m_impl->armInfo;
	m_impl->rwlock.unlock();
	return armInfo;
}

void ArmStatus::setRunningStatus(const bool &running)
{
	m_impl->rwlock.writeLock();
	m_impl->armInfo.running = running;
	m_impl->rwlock.unlock();
}

void ArmStatus::logSuccess(void)
{
	m_impl->rwlock.writeLock();
	m_impl->armInfo.stat = ARM_WORK_STAT_OK;
	m_impl->armInfo.statUpdateTime.setCurrTime();
	m_impl->armInfo.lastSuccessTime = m_impl->armInfo.statUpdateTime;
	m_impl->armInfo.numUpdate++;
	m_impl->rwlock.unlock();
}

void ArmStatus::logFailure(const string &comment,
                           const ArmWorkingStatus &status)
{
	m_impl->rwlock.writeLock();
	m_impl->armInfo.stat = status;
	m_impl->armInfo.statUpdateTime.setCurrTime();
	m_impl->armInfo.lastFailureTime = m_impl->armInfo.statUpdateTime;
	m_impl->armInfo.failureComment = comment;
	m_impl->armInfo.numUpdate++;
	m_impl->armInfo.numFailure++;
	m_impl->rwlock.unlock();
}

void ArmStatus::setArmInfo(const ArmInfo &armInfo)
{
	m_impl->rwlock.writeLock();
	m_impl->armInfo = armInfo;
	m_impl->rwlock.unlock();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
