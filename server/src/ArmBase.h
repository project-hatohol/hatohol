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

#ifndef ArmBase_h
#define ArmBase_h

#include "HatoholThreadBase.h"
#include "DBClientConfig.h"
#include "Closure.h"

class ArmBase : public HatoholThreadBase
{
public:
	typedef enum {
		UPDATE_POLLING,
		UPDATE_ITEM_REQUEST,
	} UpdateType;

public:
	ArmBase(const MonitoringServerInfo &serverInfo);
	virtual ~ArmBase();

	virtual void fetchItems(ClosureBase *closure = NULL);

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

	UpdateType getUpdateType(void) const;
	void       setUpdateType(UpdateType updateType);
	bool	   getCopyOnDemandEnabled(void) const;
	bool	   setCopyOnDemandEnabled(bool enable);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

typedef vector<ArmBase *>             ArmBaseVector;
typedef ArmBaseVector::iterator       ArmBaseVectorIterator;
typedef ArmBaseVector::const_iterator ArmBaseVectorConstIterator;

#endif // ArmBase_h
