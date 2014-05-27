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

#ifndef ArmZabbixAPI_h
#define ArmZabbixAPI_h

#include <libsoup/soup.h>
#include "ZabbixAPI.h"
#include "ArmBase.h"
#include "ItemTablePtr.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"
#include "DBClientConfig.h"
#include "DBClientZabbix.h"

const static uint64_t UNLIMITED = -1;

class ArmZabbixAPI : public ZabbixAPI, public ArmBase
{
public:
	typedef ItemTablePtr
	  (ArmZabbixAPI::*DataGetter)(const std::vector<uint64_t> &idVector);
	typedef void (ArmZabbixAPI::*TableSaver)(ItemTablePtr &tablePtr);

	static const int POLLING_DISABLED = -1;
	static const int DEFAULT_SERVER_PORT = 80;

	ArmZabbixAPI(const MonitoringServerInfo &serverInfo);
	virtual ~ArmZabbixAPI();
	ItemTablePtr getTrigger(int requestSince = 0);
	ItemTablePtr getFunctions(void);
	ItemTablePtr getItems(void);

	/**
	 * get the hosts database with Zabbix API server
	 *
	 * TODO: Write parameters and return value here.
	 */
	void getHosts(ItemTablePtr &hostsTablePtr,
	              ItemTablePtr &hostsGroupsTablePtr);

	ItemTablePtr getApplications(const std::vector<uint64_t> &appIdVector);
	ItemTablePtr getEvents(uint64_t eventIdOffset, uint64_t eventIdTill);
	uint64_t getLastEventId(void);
	virtual void onGotNewEvents(const ItemTablePtr &itemPtr);
	void getGroups(ItemTablePtr &groupsTablePtr);

protected:

	bool updateAuthTokenIfNeeded(void);
	std::string getAuthToken(void);

	SoupMessage *queryTrigger(int requestSince = 0);
	SoupMessage *queryItem(void);
	SoupMessage *queryHost(void);
	SoupMessage *queryApplication(const std::vector<uint64_t> &appIdVector);
	SoupMessage *queryEvent(uint64_t eventIdOffset, uint64_t eventIdTill);
	SoupMessage *queryGetLastEventId(void);
	SoupMessage *queryGroup(void);
	void startObject(JsonParserAgent &parser, const std::string &name);
	void startElement(JsonParserAgent &parser, int index);
	void getString(JsonParserAgent &parser, const std::string &name,
	               std::string &value);

	int pushInt(JsonParserAgent &parser, ItemGroup *itemGroup,
	            const std::string &name, ItemId itemId);
	uint64_t pushUint64(JsonParserAgent &parser, ItemGroup *itemGroup,
	                    const std::string &name, ItemId itemId);
	std::string pushString(JsonParserAgent &parser, ItemGroup *itemGroup,
	                       const std::string &name, ItemId itemId);

	void pushFunctionsCache(JsonParserAgent &parser);
	void pushFunctionsCacheOne(JsonParserAgent &parser,
	                           ItemGroup *itemGroup, int index);
	void parseAndPushTriggerData(JsonParserAgent &parser,
	                             VariableItemTablePtr &tablePtr, int index);
	void pushApplicationid(JsonParserAgent &parser, ItemGroup *itemGroup);
	void pushTriggersHostid(JsonParserAgent &parser, ItemGroup *itemGroup);
	uint64_t convertStrToUint64(const std::string strData);
	void parseAndPushItemsData(JsonParserAgent &parser,
	                           VariableItemTablePtr &tablePtr, int index);
	void parseAndPushHostsData(JsonParserAgent &parser,
	                           VariableItemTablePtr &tablePtr, int index);
	void parseAndPushApplicationsData(JsonParserAgent &parser,
	                                  VariableItemTablePtr &tablePtr,
	                                  int index);
	void parseAndPushEventsData(JsonParserAgent &parser,
	                            VariableItemTablePtr &tablePtr, int index);
	void parseAndPushGroupsData(JsonParserAgent &parser,
	                            VariableItemTablePtr &tablePtr, int index);
	void parseAndPushHostsGroupsData(JsonParserAgent &parser,
	                                 VariableItemTablePtr &tablePtr, int index);

	template<typename T>
	void updateOnlyNeededItem(
	  const ItemTable *primaryTable,
	  const ItemId pickupItemId, const ItemId checkItemId,
	  ArmZabbixAPI::DataGetter dataGetter,
	  DBClientZabbix::AbsentItemPicker absentItemPicker,
	  ArmZabbixAPI::TableSaver tableSaver);

	ItemTablePtr updateTriggers(void);
	void updateFunctions(void);
	ItemTablePtr updateItems(void);

	/**
	 * get all hosts in the ZABBIX server and save them in the replica DB.
	 */
	void updateHosts(void);

	void updateEvents(void);

	/**
	 * get all applications in the ZABBIX server and save them
	 * in the replica DB.
	 */
	void updateApplications(void);

	/**
	 * get applications with the specified IDs and save them
	 * in the replica DB.
	 * @param items
	 * A pointer to ItableTable instance that is obtained by updateItems().
	 */
	void updateApplications(const ItemTable *items);

	void updateGroups(void);

	void addApplicationsDataToDB(ItemTablePtr &applications);
	void addHostsDataToDB(ItemTablePtr &hosts);

	void makeHatoholTriggers(void);
	void makeHatoholEvents(ItemTablePtr events);
	void makeHatoholItems(ItemTablePtr events);
	void makeHatoholHostgroups(ItemTablePtr groups);
	void makeHatoholMapHostsHostgroups(ItemTablePtr hostsGroups);
	void makeHatoholHosts(ItemTablePtr hosts);

	template<typename T>
	void makeItemVector(std::vector<T> &idVector,
	                    const ItemTable *itemTable, const ItemId itemId);
	template<typename T>
	void checkObtainedItems(const ItemTable *obtainedItemTable,
	                        const std::vector<T> &requestedItemVector,
	                        const ItemId itemId);
	uint64_t getMaximumNumberGetEventPerOnce(void);

	// virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg);
	virtual bool mainThreadOneProc(void);
	virtual void onGotAuthToken(const std::string &authToken); // override

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ArmZabbixAPI_h
