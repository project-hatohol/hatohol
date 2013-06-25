/*
 * Copyright (C) 2013 Hatohol Project
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

#ifndef ArmBase_h
#define ArmBase_h

#include "HatoholThreadBase.h"
#include "DBClientConfig.h"

class ArmBase : public HatoholThreadBase
{
public:
	ArmBase(const MonitoringServerInfo &serverInfo);
	virtual ~ArmBase();

	void setPollingInterval(int sec);
	int getPollingInterval(void) const;
	int getRetryInterval(void) const;

protected:
	bool hasExitRequest(void) const;
	void requestExit(void);
	const MonitoringServerInfo &getServerInfo(void) const;
	void sleepInterruptible(int sleepTime);

	// virtual methods
	gpointer mainThread(HatoholThreadArg *arg);

	// virtual methods defined in this class
	virtual bool mainThreadOneProc(void) = 0;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ArmBase_h
