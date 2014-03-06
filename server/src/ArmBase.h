/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <string>
#include "HatoholThreadBase.h"
#include "DBClientConfig.h"
#include "ArmStatus.h"
#include "Closure.h"

class ArmBase : public HatoholThreadBase
{
public:
	typedef enum {
		UPDATE_POLLING,
		UPDATE_ITEM_REQUEST,
	} UpdateType;

public:
	ArmBase(const std::string &name,
	        const MonitoringServerInfo &serverInfo);
	virtual ~ArmBase();

	void start(void);
	void stop(void);

	const MonitoringServerInfo &getServerInfo(void) const;
	const ArmStatus &getArmStatus(void) const;

	virtual void fetchItems(ClosureBase *closure = NULL);

	void setPollingInterval(int sec);
	int getPollingInterval(void) const;
	int getRetryInterval(void) const;

	bool getCopyOnDemandEnabled(void) const;
	void setCopyOnDemandEnabled(bool enable);

	const std::string &getName(void) const;


protected:
	/**
	 * Request to exit of the thread and wait for the complition.
	 *
	 * This function is supposed to be used from a destructor of
	 * subclasses of ArmBase. The deletion of a instance of the subclasses
	 * (i.e calling a destructor) is typically done by other thread.
	 * Note that this function may block the caller thread.
	 */
	void synchronizeThreadExit(void);

	bool hasExitRequest(void) const;
	void requestExit(void);
	void sleepInterruptible(int sleepTime);

	// virtual methods
	gpointer mainThread(HatoholThreadArg *arg);

	// virtual methods defined in this class
	virtual bool mainThreadOneProc(void) = 0;

	UpdateType getUpdateType(void) const;
	void       setUpdateType(UpdateType updateType);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

typedef std::vector<ArmBase *>        ArmBaseVector;
typedef ArmBaseVector::iterator       ArmBaseVectorIterator;
typedef ArmBaseVector::const_iterator ArmBaseVectorConstIterator;

#endif // ArmBase_h
