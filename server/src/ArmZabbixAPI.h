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

#ifndef ArmZabbixAPI_h
#define ArmZabbixAPI_h

#include <libsoup/soup.h>
#include "ArmBase.h"
#include "ItemTablePtr.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"
#include "DBClientConfig.h"
#include "DBClientZabbix.h"

class ArmZabbixAPI : public ArmBase
{
public:
	typedef ItemTablePtr
	  (ArmZabbixAPI::*DataGetter)(const vector<uint64_t> &idVector);

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
	 * @param hostIdVector
	 * When this vector is empty, all hosts are requested. Otherwise,
	 * only the hosts with the IDs are requested.
	 *
	 * @return
	 * An ItemTablePtr instance that has hosts data.
	 */
	ItemTablePtr getHosts(const vector<uint64_t> &hostIdVector);

	ItemTablePtr getApplications(const vector<uint64_t> &appIdVector);
	ItemTablePtr getEvents(uint64_t eventIdOffset);
	uint64_t getLastEventId(void);

protected:
	SoupSession *getSession(void);

	/**
	 * open a session with with Zabbix API server
	 *
	 * @param msgPtr
	 * An address of SoupMessage object pointer. If this parameter is
	 * not NULL, SoupMessage object pointer is copied to this parameter.
	 * Otherwise, the object is freeed in this function and the parameter
	 * is not changed.
	 *
	 * @return
	 * true if session is oppned successfully. Otherwise, false is returned.
	 */
	bool openSession(SoupMessage **msgPtr = NULL);
	bool updateAuthTokenIfNeeded(void);
	string getAuthToken(void);

	SoupMessage *queryCommon(JsonBuilderAgent &agent);
	SoupMessage *queryTrigger(int requestSince = 0);
	SoupMessage *queryItem(void);
	SoupMessage *queryHost(const vector<uint64_t> &hostIdVector);
	SoupMessage *queryApplication(const vector<uint64_t> &appIdVector);
	SoupMessage *queryEvent(uint64_t eventIdOffset);
	SoupMessage *queryGetLastEventId(void);
	string getInitialJsonRequest(void);
	bool parseInitialResponse(SoupMessage *msg);
	void startObject(JsonParserAgent &parser, const string &name);
	void startElement(JsonParserAgent &parser, int index);
	void getString(JsonParserAgent &parser, const string &name,
	               string &value);

	int      pushInt   (JsonParserAgent &parser, ItemGroup *itemGroup,
	                    const string &name, ItemId itemId);
	uint64_t pushUint64(JsonParserAgent &parser, ItemGroup *itemGroup,
	                    const string &name, ItemId itemId);
	string   pushString(JsonParserAgent &parser, ItemGroup *itemGroup,
	                    const string &name, ItemId itemId);

	void pushFunctionsCache(JsonParserAgent &parser);
	void pushFunctionsCacheOne(JsonParserAgent &parser,
	                           ItemGroup *itemGroup, int index);
	void parseAndPushTriggerData(JsonParserAgent &parser,
	                             VariableItemTablePtr &tablePtr, int index);
	void pushApplicationid(JsonParserAgent &parser, ItemGroup *itemGroup);
	void pushTriggersHostid(JsonParserAgent &parser, ItemGroup *itemGroup);
	void parseAndPushItemsData(JsonParserAgent &parser,
	                           VariableItemTablePtr &tablePtr, int index);
	void parseAndPushHostsData(JsonParserAgent &parser,
	                           VariableItemTablePtr &tablePtr, int index);
	void parseAndPushApplicationsData(JsonParserAgent &parser,
	                                  VariableItemTablePtr &tablePtr,
	                                  int index);
	void parseAndPushEventsData(JsonParserAgent &parser,
	                            VariableItemTablePtr &tablePtr, int index);

	template<typename T>
	void updateOnlyNeededItem(
	  const ItemTable *primaryTable,
	  const ItemId pickupItemId, const ItemId checkItemId,
	  ArmZabbixAPI::DataGetter dataGetter,
	  DBClientZabbix::AbsentItemPicker absentItemPicker,
	  DBClientZabbix::TableSaver tableSaver);

	ItemTablePtr updateTriggers(void);
	void updateFunctions(void);
	ItemTablePtr updateItems(void);

	/**
	 * get all hosts in the ZABBIX server and save them in the replica DB.
	 */
	void updateHosts(void);

	/**
	 * get hosts that have one of the IDs specified by hostIdVector
	 * and save them in the replica DB.
	 * @param hostIdVector A vector of host ID.
	 */
	void updateHosts(const ItemTable *triggers);

	ItemTablePtr updateEvents(void);

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

	void makeHatoholTriggers(void);
	void makeHatoholEvents(ItemTablePtr events);
	void makeHatoholItems(ItemTablePtr events);

	template<typename T>
	void makeItemVector(vector<T> &idVector, const ItemTable *itemTable,
	                    const ItemId itemId);
	template<typename T>
	void checkObtainedItems(const ItemTable *obtainedItemTable,
	                        const vector<T> &requestedItemVector,
	                        const ItemId itemId);

	// virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg);
	virtual bool mainThreadOneProc(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ArmZabbixAPI_h
