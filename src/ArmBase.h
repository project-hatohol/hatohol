/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ArmBase_h
#define ArmBase_h

#include "AsuraThreadBase.h"
#include "DBClientConfig.h"

class ArmBase : public AsuraThreadBase
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
	gpointer mainThread(AsuraThreadArg *arg);

	// virtual methods defined in this class
	virtual bool mainThreadOneProc(void) = 0;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ArmBase_h
