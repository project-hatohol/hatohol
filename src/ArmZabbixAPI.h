/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ArmZabbixAPI_h
#define ArmZabbixAPI_h

#include <libsoup/soup.h>
#include "ArmBase.h"
#include "ItemTablePtr.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"
#include "DBClientConfig.h"

class ArmZabbixAPI : public ArmBase
{
public:
	static const int POLLING_DISABLED = -1;
	static const int DEFAULT_SERVER_PORT = 80;

	ArmZabbixAPI(const MonitoringServerInfo &serverInfo);
	virtual ~ArmZabbixAPI();
	void setPollingInterval(int sec);
	int getPollingInterval(void) const;
	void requestExit(void);
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

	ItemTablePtr getApplications(void);
	ItemTablePtr getEvents(uint64_t eventIdOffset);

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

	SoupMessage *queryCommon(JsonBuilderAgent &agent);
	SoupMessage *queryTrigger(int requestSince = 0);
	SoupMessage *queryItem(void);
	SoupMessage *queryHost(const vector<uint64_t> &hostIdVector);
	SoupMessage *queryApplication(void);
	SoupMessage *queryEvent(uint64_t eventIdOffset);
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

	ItemTablePtr updateTriggers(void);
	void updateFunctions(void);
	ItemTablePtr updateItems(void);
	void updateHosts(const vector<uint64_t> &hostIdVector);
	ItemTablePtr updateEvents(void);
	ItemTablePtr updateApplications(void);

	void makeAsuraTriggers(void);
	void makeAsuraEvents(ItemTablePtr events);
	void makeAsuraItems(ItemTablePtr events);

	void extractHostIds(vector<uint64_t> &hostIdVector,
	                    ItemTablePtr triggers);

	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);

	// virtual methods defined in this class
	virtual bool mainThreadOneProc(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ArmZabbixAPI_h
