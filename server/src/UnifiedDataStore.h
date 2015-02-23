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

#ifndef UnifiedDataStore_h
#define UnifiedDataStore_h

#include "ArmBase.h"
#include "DBTablesMonitoring.h"
#include "DBTablesAction.h"
#include "DBTablesConfig.h"
#include "Utils.h"
#include "Closure.h"
#include "DataStore.h"

struct ServerConnStatus {
	ServerIdType serverId;
	ArmInfo      armInfo;
};

typedef std::vector<ServerConnStatus>          ServerConnStatusVector;
typedef ServerConnStatusVector::iterator       ServerConnStatusVectorIterator;
typedef ServerConnStatusVector::const_iterator ServerConnStatusVectorConstIterator;

class UnifiedDataStore
{
public:
	UnifiedDataStore(void);
	virtual ~UnifiedDataStore(void);
	void reset(void);

	static UnifiedDataStore *getInstance(void);

	/**
	 * Start all virtual data stores.
	 *
	 * @param autoRun
	 * A flag to run an Arm instance soon after the store is started.
	 */
	void start(const bool &autoRun = true);

	void stop(void);
	bool getCopyOnDemandEnabled(void) const;
	void setCopyOnDemandEnabled(bool enable);

	/**
	 * Add events in the Hatohol DB and executes action if needed.
	 *
	 * @param eventList A list of EventInfo.
	 */
	void addEventList(EventInfoList &eventList);


	/*
	 *  Functions which require operation privilege
	 */

	void getTriggerList(TriggerInfoList &triggerList,
	                    const TriggersQueryOption &option);

	void addItemList(const ItemInfoList &itemList);

	/**
	 * Get the last change time of the trigger that belongs to
	 * the specified server
	 * @param serverId A target server ID.
	 * @return
	 * The last change time of tringer in unix time. If there is no
	 * trigger information, 0 is returned.
	 */
	mlpl::SmartTime getTimestampOfLastTrigger(const ServerIdType &serverId);

	HatoholError getEventList(EventInfoList &eventList,
	                          EventsQueryOption &option,
				  IncidentInfoVect *incidentVect = NULL);
	void getItemList(ItemInfoList &itemList,
	                 const ItemsQueryOption &option,
	                 bool fetchItemsSynchronously = false);
	void getApplicationVect(ApplicationInfoVect &applicationInfoVect,
	                        const ItemsQueryOption &option);
	bool fetchItemsAsync(Closure0 *closure,
	                     const ServerIdType &targetServerId = ALL_SERVERS);
	/*
	 *  We don't provide a function to get history.
	 *  Please get a DataStore by getDataStore() and use
	 *  DataStore::startOnDemandFetchHistory() directly.
	 */
	/*
	void fetchHistoryAsync(Closure1<HistoryInfoVect> *closure,
			       const ServerIdType &targetServerId,
			       const ItemIdType &itemId,
			       const time_t &beginTime,
			       const time_t &endTime);
	*/

	// Host and Hostgroup
	void getHostList(HostInfoList &hostInfoList,
	                 const HostsQueryOption &option);
	HatoholError getHostgroupInfoList(
	  HostgroupInfoList &hostgroupInfoList,
	  const HostgroupsQueryOption &option);
	HatoholError getHostgroupElementList(
	  HostgroupElementList &hostgroupElementList,
	  const HostgroupElementQueryOption &option);

	/**
	 * Add hosts. If there's hosts already exist, they will be updated.
	 *
	 * @param serverHostDef Hosts to be added/updated.
	 * @return The result of the call.
	 */
	HatoholError upsertHosts(const ServerHostDefVect &serverHostDefs);


	// Action
	HatoholError getActionList(ActionDefList &actionList,
	                           const ActionsQueryOption &option);
	HatoholError addAction(ActionDef &actionDef,
	                       const OperationPrivilege &privilege);
	HatoholError deleteActionList(const ActionIdList &actionIdList,
	                              const OperationPrivilege &privilege);
	HatoholError updateAction(ActionDef &actionDef,
	                          const OperationPrivilege &privilege);

	bool isIncidentSenderActionEnabled(void);

	size_t getNumberOfBadTriggers(const TriggersQueryOption &option,
				      TriggerSeverityType severity);
	size_t getNumberOfTriggers(const TriggersQueryOption &option);
	size_t getNumberOfGoodHosts(const TriggersQueryOption &option);
	size_t getNumberOfBadHosts(const TriggersQueryOption &option);
	size_t getNumberOfItems(const ItemsQueryOption &option,
				bool fetchItemsSynchronously = false);
	HatoholError getNumberOfMonitoredItemsPerSecond(const DataQueryOption &option,
	                                                MonitoringServerStatus &serverStatus);

	// User
	void getUserList(UserInfoList &userList,
	                 const UserQueryOption &option);
	HatoholError addUser(UserInfo &userInfo,
	                     const OperationPrivilege &privilege);
	HatoholError updateUser(UserInfo &userInfo,
	                        const OperationPrivilege &privilege);
	HatoholError updateUserFlags(OperationPrivilegeFlag &oldUserFlag,
	                             OperationPrivilegeFlag &updateUserFlag,
	                             const OperationPrivilege &privilege);
	HatoholError deleteUser(UserIdType userId,
	                        const OperationPrivilege &privilege);

	// Access List
	HatoholError getAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
	                              const AccessInfoQueryOption &option);
	HatoholError addAccessInfo(AccessInfo &userInfo,
	                           const OperationPrivilege &privilege);
	HatoholError deleteAccessInfo(AccessInfoIdType userId,
	                              const OperationPrivilege &privilege);

	// User Role
	void getUserRoleList(UserRoleInfoList &userRoleList,
	                     const UserRoleQueryOption &option);
	HatoholError addUserRole(UserRoleInfo &userRoleInfo,
	                         const OperationPrivilege &privilege);
	HatoholError updateUserRole(UserRoleInfo &userRoleInfo,
	                            const OperationPrivilege &privilege);
	HatoholError deleteUserRole(UserRoleIdType userRoleId,
	                            const OperationPrivilege &privilege);

	// ServerType
	void getServerTypes(ServerTypeInfoVect &serverTypes);

	// Server
	void getTargetServers(MonitoringServerInfoList &monitoringServers,
	                      ServerQueryOption &option,
	                      ArmPluginInfoVect *armPluginInfoVect = NULL);
	HatoholError addTargetServer(MonitoringServerInfo &svInfo,
	                             ArmPluginInfo &armPluginInfo,
	                             const OperationPrivilege &privilege,
	                             bool const &autoRun = true);
	HatoholError updateTargetServer(MonitoringServerInfo &svInfo,
	                                ArmPluginInfo &armPluginInfo,
	                                const OperationPrivilege &privilege);
	HatoholError deleteTargetServer(const ServerIdType &serverId,
	                                const OperationPrivilege &privilege);

	void getServerConnStatusVector(ServerConnStatusVector &svConnStatVec,
	                               DataQueryContext *dataQueryContext);

	// IncidentTracker
	void getIncidentTrackers(
	  IncidentTrackerInfoVect &incidentTrackerVect,
	  IncidentTrackerQueryOption &option);
	HatoholError addIncidentTracker(
	  IncidentTrackerInfo &incidentTrackerInfo,
	  const OperationPrivilege &privilege);
	HatoholError updateIncidentTracker(
	  IncidentTrackerInfo &incidentTrackerInfo,
	  const OperationPrivilege &privilege);
	HatoholError deleteIncidentTracker(
	  const IncidentTrackerIdType &incidentTrackerId,
	  const OperationPrivilege &privilege);

	// Incident
	uint64_t getLastUpdateTimeOfIncidents(
	  const IncidentTrackerIdType &trackerId);
	void addIncidentInfo(IncidentInfo &incidentInfo);

	/**
	 * get a vector of pointers of DataStore instance.
	 * This method is a wrapper of DataStoreManager::getDataStoreManager().
	 *
	 * @return
	 * A DataStoreVector instance. A used counter of each DataStore
	 * instance in it is increamented. So the caller must be call unref()
	 * for each DataStore instance.
	 */
	DataStoreVector getDataStoreVector(void);

	/**
	 * Get the data store with the specified Server ID.
	 *
	 * @param serverId A server ID.
	 * @retrun
	 * A CoutablePtr of the DataStore instance. If not found, hasData()
	 * returns false.
	 */
	DataStorePtr getDataStore(const ServerIdType &serverId);

protected:
	void fetchItems(const ServerIdType &targetServerId = ALL_SERVERS);

	void startArmIncidentTrackerIfNeeded(
	  const IncidentTrackerIdType &trackerId);
	void stopArmIncidentTrackerIfNeeded(
	  const IncidentTrackerIdType &trackerId);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // UnifiedDataStore_h
