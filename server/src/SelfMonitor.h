/*
 * Copyright (C) 2015 Project Hatohol
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

#pragma once
#include <list>
#include <memory>
#include <UsedCountable.h>
#include <Monitoring.h>
#include <Params.h>

class SelfMonitor;
typedef std::shared_ptr<SelfMonitor> SelfMonitorPtr;
typedef std::weak_ptr<SelfMonitor> SelfMonitorWeakPtr;

class SelfMonitor {
public:
	static const char *DEFAULT_SELF_MONITOR_HOST_NAME;

	typedef std::function<void (
	          SelfMonitor &monitor,
	          const TriggerStatusType &prevStatus,
	          const TriggerStatusType &newStatus)
	> EventGenerator;

	typedef std::function<void (
	          SelfMonitor &monitor, const SelfMonitor &srcMonitor,
	          const TriggerStatusType &prevStatus,
	          const TriggerStatusType &newStatus)
	> NotificationHandler;

	/**
	 * Constructor.
	 *
	 * @param serverId
	 * A serverId that is concerned with this monitor.
	 *
	 * @param triggerID
	 * A trigger ID of this monitor. If STATELESS_TRIGGER is given,
	 * the monitor is not listed in the trigger list and doesn't work as
	 * a normal trigger.
	 * This is typically used when the monitoring target doesn't
	 * have a state, but generates events.
	 *
	 * @param brief    A brief message of this monitor.
	 * @param severity A severity of this monitor.
	 */
	SelfMonitor(
	  const ServerIdType &serverId,
	  const TriggerIdType &triggerId,
	  const std::string &brief,
	  const TriggerSeverityType &severity = TRIGGER_SEVERITY_UNKNOWN);
	virtual ~SelfMonitor();

	void setEventGenerator(const TriggerStatusType &prevStatus,
	                       const TriggerStatusType &newStatus,
	                       EventGenerator generator);
	void setNotificationHandler(NotificationHandler handler);

	void update(const TriggerStatusType &status);
	TriggerStatusType getLastKnownStatus(void) const;
	void saveEvent(const std::string &brief = "",
	               const EventType &eventType = EVENT_TYPE_AUTO);

	/**
	 * Set the workable status.
	 * This feature is typically used, for example, when the propagation
	 * path has an error and the target status beyond the path cannot be
	 * obtained.
	 *
	 * @param workable
	 * When true is given, the instance works normally. This is default
	 * status. Once false is set, the trigger status becomes
	 * TRIGGER_STATUS_UNKNOWN. Then true is set, the trigger status
	 * changes to the status given by the last update().
	 * Note that these changes invokes the processings and callbacks
	 * similary to the call of update().
	 *
	 */
	void setWorkable(const bool &workable);

	/**
	 * Add a SelfMonitor instance. The given monitor instance here can be
	 * deleted while this instance is alive without any operation to this
	 * instance.
	 *
	 * @param monitor A shared_pointer of SelfMonitor instance.
	 * When update() of this instance is called, the given monitor's
	 * onNotified is invoked().
	 */
	void addNotificationListener(SelfMonitorPtr monitor);

	/**
	 * Called in update(). The default behavior is calling the
	 * registered handler set by setEventGenerator().
	 * If this method is overridden, the behavior doesn't naturally.
	 */
	virtual void generateEvent(const TriggerStatusType &prevStatus,
	                           const TriggerStatusType &newStatus);

	virtual void onUpdated(const TriggerStatusType &prevStatus,
	                       const TriggerStatusType &newStatus);

	/**
	 * This method is called back when the status of the SelfMonitor
	 * instance of which this instance has called addNotificationLister()
	 * is updated.
	 *
	 * The default behavior is calling a handler registered by
	 * setNotificaitionHandler(). Don't override this method,
	 * if the mechanism is used.
	 *
	 * @param srcMonitor A SelfMonitor instance of the the status chagned
	 * @param prevStatus A previous status.
	 * @param newStatus  A new status.
	 */
	virtual void onNotified(const SelfMonitor &srcMonitor,
	                        const TriggerStatusType &prevStatus,
	                        const TriggerStatusType &newStatus);
private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

typedef std::list<SelfMonitorWeakPtr> SelfMonitorWeakPtrList;

