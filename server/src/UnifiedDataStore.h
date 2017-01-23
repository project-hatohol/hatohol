/*
 * Copyright (C) 2013-2016 Project Hatohol
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

#include <memory>
#include "ArmBase.h"
#include "DBTablesMonitoring.h"
#include "DBTablesAction.h"
#include "DBTablesConfig.h"
#include "Utils.h"
#include "Closure.h"
#include "DataStore.h"
#include "HostInfoCache.h"

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

	/**
	 * Add events in the Hatohol DB and executes action if needed.
	 *
	 * @param eventList A list of EventInfo.
	 * @param hooks     Transaction hook functions.
	 */
	void addEventList(EventInfoList &eventList,
	                  DBAgent::TransactionHooks *hooks = NULL);

	void addItemList(const ItemInfoList &itemList);
	void syncItems(const ItemInfoList &itemList,
	               const ServerIdType &serverId);

	void addMonitoringServerStatus(
	  const MonitoringServerStatus &serverStatus);


	/*
	 *  Functions which require operation privilege
	 */

	void getTriggerList(TriggerInfoList &triggerList,
	                    const TriggersQueryOption &option);
	void getTriggerBriefList(std::list<std::string> &triggerBriefList,
	                         const TriggersQueryOption &option);

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
	void getItemCategoryNames(std::vector<std::string> &itemCategoryNames,
	                          const ItemsQueryOption &option);
	bool fetchItemsAsync(Closure0 *closure,
	                     const ItemsQueryOption &option);
	bool fetchTriggerAsync(Closure0 *closure,
			       const TriggersQueryOption &option);

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
	HatoholError getServerHostDefs(ServerHostDefVect &svHostDefVect,
	                               const HostsQueryOption &option);

	HatoholError getHostgroups(HostgroupVect &hostgroups,
	                           const HostgroupsQueryOption &option);
	HatoholError getServerHostGrpSetMap(
	  ServerHostGrpSetMap &serverHostGrpSetMap,
	  const HostgroupsQueryOption &option);

	/**
	 * Add hosts. If there's hosts already exist, they will be updated.
	 *
	 * See also DBTablesHost::upsert().
	 *
	 * @param serverHostDef A host to be added/updated.
	 * @param hostId
	 * If this parameter is not NULL, the added/updated host ID is written
	 * to this area.
	 *
	 * @return The result of the call.
	 */
	HatoholError upsertHost(const ServerHostDef &serverHostDef,
	                        HostIdType *hostId = NULL);

	/**
	 * Add hosts. Already exisiting hosts are updated.
	 *
	 * See also DBTablesHost::upsert().
	 *
	 * @param serverHostDef Hosts to be added/updated.
	 * @param hostHostIdMapPtr
	 * If this parameter is not NULL, the address of the ServerHostDef
	 * and the corresponding host IDs are stored in it.
	 * @param hook Transaction hook functions.
	 *
	 * @return The result of the call.
	 */
	HatoholError
	  upsertHosts(const ServerHostDefVect &serverHostDefs,
	              HostHostIdMap *hostHostIdMapPtr = NULL,
	              DBAgent::TransactionHooks *hooks = NULL);

	/**
	 * call upsertHosts for given hosts and update HostInfoCache.
	 */
	HatoholError syncHosts(
	  const ServerHostDefVect &svHostDefs, const ServerIdType &serverId,
	  HostInfoCache &hostInfoCache,
	  DBAgent::TransactionHooks *hooks = NULL);
	bool wasStoredHostsChanged(void);
	size_t getNumberOfHosts(HostsQueryOption &option);

	HatoholError upsertHostgroups(const HostgroupVect &hostgroups,
	                              DBAgent::TransactionHooks *hooks = NULL);
	HatoholError syncHostgroups(const HostgroupVect &hostgroups,
	                            const ServerIdType &serverId,
	                            DBAgent::TransactionHooks *hooks = NULL);

	HatoholError upsertHostgroupMembers(
	  const HostgroupMemberVect &hostgroupMembers,
	  DBAgent::TransactionHooks *hooks = NULL);
	HatoholError syncHostgroupMembers(
	  const HostgroupMemberVect &hostgroups,
	  const ServerIdType &serverId,
	  DBAgent::TransactionHooks *hooks = NULL);

	HatoholError getHostgroupMembers(
	  HostgroupMemberVect &hostgroupMembers,
	  const HostgroupMembersQueryOption &option);

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
	void addTriggers(const TriggerInfoList &triggerInfoList,
	                 DBAgent::TransactionHooks *hooks = NULL);
	HatoholError syncTriggers(const TriggerInfoList &triggerInfoList,
	                          const ServerIdType &serverId,
	                          DBAgent::TransactionHooks *hooks = NULL);
	size_t getNumberOfGoodHosts(const TriggersQueryOption &option);
	size_t getNumberOfBadHosts(const TriggersQueryOption &option);
	size_t getNumberOfItems(const ItemsQueryOption &option,
				bool fetchItemsSynchronously = false);
	HatoholError getNumberOfMonitoredItemsPerSecond(const DataQueryOption &option,
	                                                MonitoringServerStatus &serverStatus);
	size_t getNumberOfEvents(const EventsQueryOption &option);
	size_t getNumberOfHostsWithSpecifiedEvents(const EventsQueryOption &option);
	HatoholError getEventSeverityStatistics(
	  std::vector<DBTablesMonitoring::EventSeverityStatistics> &importantEventGroupVect,
	  const EventsQueryOption &option);

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
	HatoholError getIncidents(IncidentInfoVect &incidentInfoVect,
				  const IncidentsQueryOption &option);
	uint64_t getLastUpdateTimeOfIncidents(
	  const IncidentTrackerIdType &trackerId);
	void addIncidentInfo(IncidentInfo &incidentInfo);
	HatoholError updateIncidentInfo(IncidentInfo &incidentInfo);

	HatoholError addIncidentHistory(
	  IncidentHistory &incidentHistory);
	HatoholError updateIncidentHistory(
	  IncidentHistory &incidentHistory);
	HatoholError getIncidentHistories(
	  std::list<IncidentHistory> &IncidentHistoriesList,
	  const IncidentHistoriesQueryOption &option);

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
	std::shared_ptr<DataStore> getDataStore(const ServerIdType &serverId);
	bool getIncidentTrackerInfo(const IncidentTrackerIdType &trackerId,
                                    IncidentTrackerInfo &trackerInfo);


	struct SystemInfo {
		DBTablesMonitoring::SystemInfo monitoring;
	};
	void getSystemInfo(SystemInfo &systemInfo,
	                   const DataQueryOption &option);
	// SeverityRank
	HatoholError upsertSeverityRank(SeverityRankInfo &severityRankInfo,
	                                const OperationPrivilege privilege);
	HatoholError updateSeverityRank(SeverityRankInfo &severityRankInfo,
	                                const OperationPrivilege privilege);
	HatoholError getSeverityRanks(SeverityRankInfoVect &severityRankInfoVect,
	                              const SeverityRankQueryOption &option);
	HatoholError deleteSeverityRanks(std::list<SeverityRankIdType> &idList,
	                                 const OperationPrivilege privilege);
	// CustomIncidentStatus
	HatoholError upsertCustomIncidentStatus(
	  CustomIncidentStatus &customIncidentStatus,
	  const OperationPrivilege privilege);
	HatoholError updateCustomIncidentStatus(
	  CustomIncidentStatus &customIncidentStatus,
	  const OperationPrivilege privilege);
	HatoholError getCustomIncidentStatuses(
	  std::vector<CustomIncidentStatus> &customIncidentStatusVect,
	  const CustomIncidentStatusesQueryOption &option);
	HatoholError deleteCustomIncidentStatuses(
	  std::list<CustomIncidentStatusIdType> &idList,
	  const OperationPrivilege privilege);
	void getCustomIncidentStatusesCache(
	  std::map<std::string, CustomIncidentStatus> &customIncidentStatusMap);

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
