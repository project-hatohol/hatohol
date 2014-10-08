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

#ifndef HapProcessCeilometer_h
#define HapProcessCeilometer_h

#include <string>
#include <SmartTime.h>
#include <Reaper.h>
#include <libsoup/soup.h>
#include "JSONParser.h"
#include "HapProcessStandard.h"
#include "Monitoring.h"

class HapProcessCeilometer : public HapProcessStandard {
public:
	HapProcessCeilometer(int argc, char *argv[]);
	virtual ~HapProcessCeilometer();

protected:
	typedef std::map<const hfl::SmartTime, const ItemGroupPtr> AlarmTimeMap;
	typedef AlarmTimeMap::const_iterator AlarmTimeMapConstIterator;

	bool canSkipAuthentification(void);
	HatoholError updateAuthTokenIfNeeded(void);
	bool parseReplyToknes(SoupMessage *msg);

	struct HttpRequestArg;
	HatoholError sendHttpRequest(HttpRequestArg &arg);

	HatoholError getInstanceList(void);
	HatoholError parseReplyInstanceList(SoupMessage *msg,
	                                    VariableItemTablePtr &tablePtr);
	HatoholError parseInstanceElement(JSONParser &parser,
	                                  VariableItemTablePtr &tablePtr,
                                          const unsigned int &index);

	HatoholError getAlarmList(void);
	HatoholError parseReplyGetAlarmList(SoupMessage *msg,
	                                    VariableItemTablePtr &tablePtr);
	HatoholError parseAlarmElement(JSONParser &parser,
	                               VariableItemTablePtr &tablePtr,
                                       const unsigned int &index);
	TriggerStatusType parseAlarmState(const std::string &state);
	hfl::SmartTime parseStateTimestamp(const std::string &stateTimestamp);
	uint64_t generateHashU64(const std::string &str);

	HatoholError getAlarmHistories(void);
	HatoholError getAlarmHistory(const unsigned int &index);
	std::string  getHistoryQueryOption(const hfl::SmartTime &lastTime);
	HatoholError parseReplyGetAlarmHistory(SoupMessage *msg,
	                                       AlarmTimeMap &alarmTimeMap);
	HatoholError parseReplyGetAlarmHistoryElement(
	  JSONParser &parser, AlarmTimeMap &alarmTimeMap,
	  const unsigned int &index);
	EventType parseAlarmHistoryDetail(const std::string &detail);

	HatoholError parseAlarmHost(JSONParser &parser, HostIdType &hostId);
	HatoholError parseAlarmHostEach(JSONParser &parser, HostIdType &hostId,
	                                const unsigned int &index);

	bool startObject(JSONParser &parser, const std::string &name);
	bool read(JSONParser &parser, const std::string &member,
	          std::string &dest);
	bool read(JSONParser &parser, const std::string &member,
	          double &dest);
	bool parserEndpoints(JSONParser &parser, const unsigned int &index);

	virtual HatoholError acquireData(void) override;
	virtual HatoholError fetchItem(void) override;
	HatoholError fetchItemsOfInstance(
	  VariableItemTablePtr &tablePtr, const std::string &instanceId);
	HatoholError parserResourceLink(
	  JSONParser &parser, VariableItemTablePtr &tablePtr,
	  const unsigned int &index, const std::string &instanceId);
	HatoholError getResource(
	  VariableItemTablePtr &tablePtr, const std::string &url,
	  const std::string &instanceId);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // HapProcessCeilometer_h
