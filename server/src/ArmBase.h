/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#ifndef ArmBase_h
#define ArmBase_h

#include <string>
#include "HatoholThreadBase.h"
#include "DBTablesConfig.h"
#include "ArmStatus.h"
#include "Closure.h"

class ArmBase : public HatoholThreadBase
{
public:
	typedef enum {
		UPDATE_POLLING,
		UPDATE_ITEM_REQUEST,
	} UpdateType;

	typedef enum {
		COLLECT_NG_PARSER_ERROR = 0,
		COLLECT_NG_DISCONNECT_ZABBIX,
		COLLECT_NG_DISCONNECT_NAGIOS,
		COLLECT_NG_DISCONNECT_REDMINE,
		COLLECT_NG_INTERNAL_ERROR,
		NUM_COLLECT_NG_KIND,
		COLLECT_OK,
	} ArmPollingResult;

	struct ArmResultTriggerInfo {
		TriggerStatusType statusType;
		TriggerIdType triggerId;
		std::string msg;
	};

public:
	ArmBase(const std::string &name,
	        const MonitoringServerInfo &serverInfo);
	virtual ~ArmBase();

	void start(void);
	virtual void waitExit(void) override;

	const MonitoringServerInfo &getServerInfo(void) const;
	const ArmStatus &getArmStatus(void) const;

	virtual bool isFetchItemsSupported(void) const;
	virtual void fetchItems(ClosureBase *closure = NULL);

	void setPollingInterval(int sec);
	int getPollingInterval(void) const;
	int getRetryInterval(void) const;

	bool getCopyOnDemandEnabled(void) const;
	void setCopyOnDemandEnabled(bool enable);

	const std::string &getName(void) const;

	void setServerConnectStatus(const ArmPollingResult &type);

	void registerAvailableTrigger(const ArmPollingResult &type,
				      const TriggerIdType  &trrigerId,
				      const HatoholError   &hatholError);
protected:
	/**
	 * Request to exit the thread and wait for the complition.
	 *
	 * This function is supposed to be used from a destructor of
	 * subclasses of ArmBase. The deletion of a instance of the subclasses
	 * (i.e calling a destructor) is typically done by other thread.
	 * Note that this function may block the caller thread.
	 */
	void requestExitAndWait(void);

	bool hasExitRequest(void) const;
	void requestExit(void);
	void sleepInterruptible(int sleepTime);

	// virtual methods
	gpointer mainThread(HatoholThreadArg *arg);

	// virtual methods defined in this class
	virtual ArmPollingResult mainThreadOneProc(void) = 0;

	UpdateType getUpdateType(void) const;
	void       setUpdateType(UpdateType updateType);

	void getArmStatus(ArmStatus *&armStatus);
	void setFailureInfo(
	  const std::string &comment,
	  const ArmWorkingStatus &status = ARM_WORK_STAT_FAILURE);
	
	void createTriggerInfo(const ArmResultTriggerInfo &resTrigger,
			       TriggerInfoList &triggerInfoList);
	void createEventInfo(const ArmResultTriggerInfo &resTrigger,
			     EventInfoList &eventInfoList);
	void setInitialTrrigerStatus(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

typedef std::vector<ArmBase *>        ArmBaseVector;
typedef ArmBaseVector::iterator       ArmBaseVectorIterator;
typedef ArmBaseVector::const_iterator ArmBaseVectorConstIterator;

#endif // ArmBase_h
