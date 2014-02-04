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

#ifndef UnifiedDataStore_h
#define UnifiedDataStore_h

#include "ArmBase.h"
#include "DBClientHatohol.h"
#include "DBClientAction.h"
#include "DBClientConfig.h"
#include "Utils.h"
#include "Closure.h"

class UnifiedDataStore
{
public:
	UnifiedDataStore(void);
	virtual ~UnifiedDataStore(void);

	static UnifiedDataStore *getInstance(void);
	virtual void parseCommandLineArgument(CommandLineArg &cmdArg);
	virtual void start(void);
	virtual void stop(void);
	virtual bool getCopyOnDemandEnabled(void) const;
	virtual void setCopyOnDemandEnabled(bool enable);

	/**
	 * Add events in the Hatohol DB and executes action if needed. 
	 * 
	 * @param eventList A list of EventInfo.
	 */
	virtual void addEventList(const EventInfoList &eventList);


	/*
	 *  Functions which require operation privilege
	 */

	virtual void getTriggerList(TriggerInfoList &triggerList,
				    const TriggersQueryOption &option,
	                            uint64_t targetTriggerId = ALL_TRIGGERS);
	virtual HatoholError getEventList(EventInfoList &eventList,
	                                  EventsQueryOption &option);
	virtual void getItemList(ItemInfoList &itemList,
	                         const ItemsQueryOption &option,
	                         uint64_t targetItemId = ALL_ITEMS,
				 bool fetchItemsSynchronously = false);
	virtual bool fetchItemsAsync(
	  ClosureBase *closure,
	  const ServerIdType &targetServerId = ALL_SERVERS);
	virtual void getHostList(HostInfoList &hostInfoList,
				 const HostsQueryOption &option);
	virtual HatoholError getActionList(ActionDefList &actionList,
	                                   const OperationPrivilege &privilege);
	virtual HatoholError addAction(ActionDef &actionDef,
	                               const OperationPrivilege &privilege);
	virtual HatoholError deleteActionList(
	  const ActionIdList &actionIdList,
	  const OperationPrivilege &privilege);
	virtual HatoholError getHostgroupInfoList(
	  HostgroupInfoList &hostgroupInfoList,
	  const HostgroupsQueryOption &option);
	virtual HatoholError getHostgroupElementList(
	  HostgroupElementList &hostgroupElementList,
	  const HostgroupElementQueryOption &option);

	virtual size_t getNumberOfTriggers
	                 (const TriggersQueryOption &option,
	                  TriggerSeverityType severity);
	virtual size_t getNumberOfGoodHosts(const HostsQueryOption &option);
	virtual size_t getNumberOfBadHosts(const HostsQueryOption &option);

	virtual void getUserList(UserInfoList &userList,
                                 const UserQueryOption &option);
	virtual HatoholError addUser(
	  UserInfo &userInfo, const OperationPrivilege &privilege);
	virtual HatoholError updateUser(
	  UserInfo &userInfo, const OperationPrivilege &privilege);
	virtual HatoholError deleteUser(
	  UserIdType userId, const OperationPrivilege &privilege);
	virtual HatoholError getAccessInfoMap(
	  ServerAccessInfoMap &srvAccessInfoMap,
	  const AccessInfoQueryOption &option);
	virtual HatoholError addAccessInfo(
	  AccessInfo &userInfo, const OperationPrivilege &privilege);
	virtual HatoholError deleteAccessInfo(
	  AccessInfoIdType userId, const OperationPrivilege &privilege);
	virtual void getUserRoleList(UserRoleInfoList &userRoleList,
                                     const UserRoleQueryOption &option);
	virtual HatoholError addUserRole(
	  UserRoleInfo &userRoleInfo, const OperationPrivilege &privilege);
	virtual HatoholError updateUserRole(
	  UserRoleInfo &userRoleInfo, const OperationPrivilege &privilege);
	virtual HatoholError deleteUserRole(
	  UserRoleIdType userRoleId, const OperationPrivilege &privilege);
	virtual void getTargetServers(
	  MonitoringServerInfoList &monitoringServers,
	  ServerQueryOption &option);
	virtual HatoholError addTargetServer(
	  MonitoringServerInfo &svInfo, const OperationPrivilege &privilege);
	virtual HatoholError deleteTargetServer(
	  const ServerIdType &serverId, const OperationPrivilege &privilege);

protected:
	virtual void fetchItems(
	  const ServerIdType &targetServerId = ALL_SERVERS);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // UnifiedDataStore_h
