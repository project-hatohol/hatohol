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

#ifndef HatoholArmPluginGate_h
#define HatoholArmPluginGate_h

#include "HatoholThreadBase.h"
#include "DataStore.h"
#include "UsedCountablePtr.h"

class HatoholArmPluginGate : public DataStore, public HatoholThreadBase {
public:
	HatoholArmPluginGate(const MonitoringServerInfo &serverInfo);

	/**
	 * Start an initiation. This typically launch a plugin process.
	 */
	virtual void start(void);

	/**
	 * Reutrn an ArmStatus instance.
	 *
	 * Note that the caller don't make a copy of the returned value on a
	 * stack. It causes the double free.
	 *
	 * @return an ArmStatusInstance.
	 */
	const ArmStatus &getArmStatus(void) const;

	// This is dummy and this virtual method should be removed
	virtual ArmBase &getArmBase(void) // override
	{
		return *((ArmBase *)NULL);
	}

	// virtual methods

	/**
	 * Wait for a complete exit of the thread.
	 */
	virtual void waitExit(void); // override

	gpointer mainThread(HatoholThreadArg *arg); // override

protected:
	// To avoid an instance from being created on a stack.
	virtual ~HatoholArmPluginGate();

private:
	struct PrivateContext;
	PrivateContext *m_ctx;;
};

typedef UsedCountablePtr<HatoholArmPluginGate> HatoholArmPluginGatePtr;

#endif // HatoholArmPluginGate_h

