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

#ifndef ZabbixAPI_h
#define ZabbixAPI_h

#include <string>
#include <libsoup/soup.h>
#include "MonitoringServerInfo.h"
#include "ItemTablePtr.h"
#include "JsonBuilderAgent.h"
#include "JsonParserAgent.h"

enum EventIdClass {
	FIRST_EVENT_ID,
	LAST_EVENT_ID
};

class ZabbixAPI
{
public:
	ZabbixAPI(void);
	virtual ~ZabbixAPI();

	static const uint64_t EVENT_ID_NOT_FOUND;

protected:
	const static uint64_t UNLIMITED = -1;
	void setMonitoringServerInfo(const MonitoringServerInfo &serverInfo);

	/**
	 * Called when the authtoken is updated.
	 */
	virtual void onUpdatedAuthToken(const std::string &authToken);

	/**
	 * Get the API version of the target ZABBIX server.
	 * Note that this method is NOT MT-safe.
	 *
	 * @retrun An API version.
	 */
	const std::string &getAPIVersion(void);

	/**
	 * Compare the API version of the target Zabbix server
	 * with the specified version.
	 *
	 * @param major A major version. Ex. 1 if version is 1.2.3.
	 * @param minor A minor version. Ex. 2 if version is 1.2.3.
	 * @param micro A micro version. Ex. 3 if version is 1.2.3.
	 *
	 * @return
	 * true if the API version of the server is equal to or greater than
	 * that of specified version. Otherwise false is returned.
	 */
	bool checkAPIVersion(int major, int minor, int micro);

	/**
	 * Open a session with with Zabbix API server
	 *
	 * @param msgPtr
	 * An address of a SoupMessage object pointer. If this parameter is
	 * not NULL, a SoupMessage object pointer is copied to this parameter.
	 * Otherwise, the object is freeed internally. And the parameter is
	 * not changed.
	 *
	 * @return
	 * true if a session is oppned successfully. Otherwise, false is
	 * returned.
	 */
	bool openSession(SoupMessage **msgPtr = NULL);

	SoupSession *getSession(void);
	bool updateAuthTokenIfNeeded(void);
	std::string getAuthToken(void);
	void clearAuthToken(void);

	/**
	 * Get the triggers.
	 *
	 * @param requestSince
	 * Triggers with timestamp after this parameter will be returned.
	 *
	 * @return The obtained triggers as an ItemTable format.
	 */
	ItemTablePtr getTrigger(int requestSince = 0);

	/**
	 * Get the items.
	 *
	 * @return The obtained items as an ItemTable format.
	 */
	ItemTablePtr getItems(void);

	/**
	 * Get the hosts and the host groups.
	 *
	 * @param hostsTablePtr
	 * A ItemTablePtr the obtained hosts are stored in.
	 *
	 * @param hostsGroupsTablePtr
	 * A ItemTablePtr the obtained host groups are stored in.
	 */
	void getHosts(ItemTablePtr &hostsTablePtr,
	              ItemTablePtr &hostsGroupsTablePtr);

	/**
	 * Get the groups.
	 *
	 * @param groupsTablePtr
	 * A ItemTablePtr the obtained groups are stored in.
	 */
	void getGroups(ItemTablePtr &groupsTablePtr);

	/**
	 * Get the applications
	 *
	 * @param appIdVector
	 * A vector filled with required application IDs.
	 *
	 * @return The obtained triggers as an ItemTable format.
	 */
	ItemTablePtr getApplications(const std::vector<uint64_t> &appIdVector);

	/**
	 * Get the applications
	 *
	 * @param eventIdOffset
	 * The first event ID to be obtained.
	 *
	 * @param eventIdTill
	 * The last event ID to be obtained.
	 *
	 * @return The obtained events as an ItemTable format.
	 */
	ItemTablePtr getEvents(uint64_t eventIdOffset, uint64_t eventIdTill);

	/**
	 * Get the first or last event id the target Zabbix server has.
	 *
	 * @param type
	 * A type of EventIdClass.
	 *
	 * @return The first or last event ID.
	 */
	uint64_t getFirstOrLastEventId(const EventIdClass &type);

	/**
	 * Get the triggers.
	 *
	 * @param requestSince
	 * Triggers with timestamp after this parameter will be returned.
	 *
	 * @return
	 * A SoupMessage object with the raw Zabbix servers's response.
	 */
	SoupMessage *queryTrigger(int requestSince = 0);

	/**
	 * Get the triggers.
	 *
	 * @return
	 * A SoupMessage object with the raw Zabbix servers's response.
	 */
	SoupMessage *queryItem(void);

	/**
	 * Get the hosts.
	 *
	 * @return
	 * A SoupMessage object with the raw Zabbix servers's response.
	 */
	SoupMessage *queryHost(void);

	/**
	 * Get the groups.
	 *
	 * @return
	 * A SoupMessage object with the raw Zabbix servers's response.
	 */
	SoupMessage *queryGroup(void);

	/**
	 * Get the applications.
	 *
	 * @return
	 * A SoupMessage object with the raw Zabbix servers's response.
	 */
	SoupMessage *queryApplication(const std::vector<uint64_t> &appIdVector);

	/**
	 * Get the events.
	 *
	 * @return
	 * A SoupMessage object with the raw Zabbix servers's response.
	 */
	SoupMessage *queryEvent(uint64_t eventIdOffset, uint64_t eventIdTill);

	/**
	 * Get the first or last event ID the target Zabbix server has.
	 *
	 * @param type
	 * A type of EventIdClass.
	 *
	 * @return
	 * A SoupMessage object with the raw Zabbix servers's response.
	 */
	SoupMessage *queryFirstOrLastEventId(const EventIdClass &type);

	/**
	 * Get the functions.
	 * Actually, the body of 'functions' is objtained in the prior call of
	 * getTrigger(). So the caller must be call getTrigger() before this
	 * method.
	 *
	 * @return The obtained functions as an ItemTable format.
	 */
	ItemTablePtr getFunctions(void);

	SoupMessage *queryCommon(JsonBuilderAgent &agent);
	SoupMessage *queryAPIVersion(void);
	std::string getInitialJsonRequest(void);
	bool parseInitialResponse(SoupMessage *msg);
	void startObject(JsonParserAgent &parser, const std::string &name);
	void startElement(JsonParserAgent &parser, const int &index);

	void getString(JsonParserAgent &parser, const std::string &name,
	               std::string &value);
	int pushInt(JsonParserAgent &parser, ItemGroup *itemGroup,
	            const std::string &name, const ItemId &itemId);
	uint64_t pushUint64(JsonParserAgent &parser, ItemGroup *itemGroup,
	                    const std::string &name, const ItemId &itemId);
	std::string pushString(JsonParserAgent &parser, ItemGroup *itemGroup,
	                       const std::string &name, const ItemId &itemId);
	void parseAndPushTriggerData(
	  JsonParserAgent &parser,
	  VariableItemTablePtr &tablePtr, const int &index);
	void parseAndPushItemsData(
	  JsonParserAgent &parser,
	  VariableItemTablePtr &tablePtr, const int &index);
	void parseAndPushHostsData(
	  JsonParserAgent &parser,
	  VariableItemTablePtr &tablePtr, const int &index);
	void parseAndPushHostsGroupsData(
	  JsonParserAgent &parser,
	  VariableItemTablePtr &tablePtr, const int &index);
	void parseAndPushGroupsData(
	  JsonParserAgent &parser,
	  VariableItemTablePtr &tablePtr, const int &index);
	void parseAndPushApplicationsData(
	  JsonParserAgent &parser,
	  VariableItemTablePtr &tablePtr, const int &index);
	void parseAndPushEventsData(
	  JsonParserAgent &parser,
	  VariableItemTablePtr &tablePtr, const int &index);

	void pushTriggersHostid(JsonParserAgent &parser, ItemGroup *itemGroup);
	void pushApplicationid(JsonParserAgent &parser, ItemGroup *itemGroup);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ZabbixAPI_h
