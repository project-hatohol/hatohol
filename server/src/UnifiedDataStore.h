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

#ifndef UnifiedDataStore_h
#define UnifiedDataStore_h

#include "ArmBase.h"
#include "DBClientHatohol.h"
#include "DBClientAction.h"
#include "DBClientConfig.h"
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
	void parseCommandLineArgument(CommandLineArg &cmdArg);

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
	void addEventList(const EventInfoList &eventList);


	/*
	 *  Functions which require operation privilege
	 */

	void getTriggerList(TriggerInfoList &triggerList,
	                    const TriggersQueryOption &option);
	mlpl::SmartTime getTimestampOfLastTrigger(const ServerIdType serverId);
	HatoholError getEventList(EventInfoList &eventList,
	                          EventsQueryOption &option);
	void getItemList(ItemInfoList &itemList,
	                 const ItemsQueryOption &option,
	                 bool fetchItemsSynchronously = false);
	bool fetchItemsAsync(ClosureBase *closure,
	                     const ServerIdType &targetServerId = ALL_SERVERS);

	// Host and Hostgroup
	void getHostList(HostInfoList &hostInfoList,
	                 const HostsQueryOption &option);
	HatoholError getHostgroupInfoList(
	  HostgroupInfoList &hostgroupInfoList,
	  const HostgroupsQueryOption &option);
	HatoholError getHostgroupElementList(
	  HostgroupElementList &hostgroupElementList,
	  const HostgroupElementQueryOption &option);


	// Action
	HatoholError getActionList(ActionDefList &actionList,
	                           const OperationPrivilege &privilege);
	HatoholError addAction(ActionDef &actionDef,
	                       const OperationPrivilege &privilege);
	HatoholError deleteActionList(const ActionIdList &actionIdList,
	                              const OperationPrivilege &privilege);

	size_t getNumberOfBadTriggers(const TriggersQueryOption &option,
				      TriggerSeverityType severity);
	size_t getNumberOfTriggers(const TriggersQueryOption &option);
	size_t getNumberOfGoodHosts(const TriggersQueryOption &option);
	size_t getNumberOfBadHosts(const TriggersQueryOption &option);
	HatoholError getNumberOfMonitoredItemsPerSecond(const DataQueryOption &option,
	                                                MonitoringServerStatus &serverStatus);

	// User
	void getUserList(UserInfoList &userList,
	                 const UserQueryOption &option);
	HatoholError addUser(UserInfo &userInfo,
	                     const OperationPrivilege &privilege);
	HatoholError updateUser(UserInfo &userInfo,
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

	// Server
	void getTargetServers(MonitoringServerInfoList &monitoringServers,
	                      ServerQueryOption &option);
	HatoholError addTargetServer(MonitoringServerInfo &svInfo,
	                             ArmPluginInfo &armPluginInfo,
	                             const OperationPrivilege &privilege,
	                             bool const &autoRun = true);
	HatoholError updateTargetServer(MonitoringServerInfo &svInfo,
	                                const OperationPrivilege &privilege);
	HatoholError deleteTargetServer(const ServerIdType &serverId,
	                                const OperationPrivilege &privilege);

	void getServerConnStatusVector(ServerConnStatusVector &svConnStatVec,
	                               DataQueryContext *dataQueryContext);

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

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // UnifiedDataStore_h
