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

#ifndef HatoholArmPluginGateHAPI2_h
#define HatoholArmPluginGateHAPI2_h

#include "DataStore.h"
#include "HatoholArmPluginInterfaceHAPI2.h"
#include "JSONBuilder.h"
#include "DBTablesLastInfo.h"

constexpr const char *DO_NOT_ASSOCIATE_TRIGGER_ID =
  SPECIAL_TRIGGER_ID_PREFIX "DO_NOT_ASSOCIATE_TRIGGER";

class HatoholArmPluginGateHAPI2 : public DataStore, public HatoholArmPluginInterfaceHAPI2 {
public:
	HatoholArmPluginGateHAPI2(const MonitoringServerInfo &serverInfo,
				  const bool &autoStart = true);

	virtual const MonitoringServerInfo
	  &getMonitoringServerInfo(void) const override;
	virtual const ArmStatus &getArmStatus(void) const override;
	virtual void start(void) override;
	bool isEstablished(void);

	virtual bool isFetchItemsSupported(void);
	virtual bool startOnDemandFetchItems(
	  const LocalHostIdVector &hostIds = {},
	  Closure0 *closure = NULL) override;
	virtual void startOnDemandFetchHistory(
	  const ItemInfo &itemInfo,
	  const time_t &beginTime,
	  const time_t &endTime,
	  Closure1<HistoryInfoVect> *closure = NULL) override;
	virtual bool startOnDemandFetchTriggers(
	  const LocalHostIdVector &hostIds = {},
	  Closure0 *closure = NULL) override;
	virtual bool startOnDemandFetchEvents(
	  const std::string &lastInfo,
	  const size_t count,
	  const bool ascending = true,
	  Closure0 *closure = NULL) override;
	static bool convertLastInfoStrToType(const std::string &type,
	                                     LastInfoType &lastInfoType);

protected:
	virtual ~HatoholArmPluginGateHAPI2();
	void upsertLastInfo(std::string lastInfoValue, LastInfoType type);
	void updateSelfMonitoringTrigger(bool hasError,
	                                 const HAPI2PluginCollectType &type,
	                                 const HAPI2PluginErrorCode &errorCode);
	virtual void onSetPluginInitialInfo(void) override;
	virtual void onConnect(void) override;
	virtual void onConnectFailure(void) override;
	void setPluginAvailableTrigger(const HAPI2PluginCollectType &type,
				       const TriggerIdType &trrigerId,
				       const HatoholError &hatoholError);
	void setPluginConnectStatus(const HAPI2PluginCollectType &type,
				    const HAPI2PluginErrorCode &errorCode);

private:
	std::string procedureHandlerExchangeProfile(JSONParser &parser);
	std::string procedureHandlerMonitoringServerInfo(JSONParser &parser);
	std::string procedureHandlerLastInfo(JSONParser &parser);
	std::string procedureHandlerPutItems(JSONParser &parser);
	std::string procedureHandlerPutHistory(JSONParser &parser);
	std::string procedureHandlerPutHosts(JSONParser &parser);
	std::string procedureHandlerPutHostGroups(JSONParser &parser);
	std::string procedureHandlerPutHostGroupMembership(JSONParser &parser);
	std::string procedureHandlerPutTriggers(JSONParser &parser);
	std::string procedureHandlerPutEvents(JSONParser &parser);
	std::string procedureHandlerPutHostParents(JSONParser &parser);
	std::string procedureHandlerPutArmInfo(JSONParser &parser);

public:
	static bool parseTimeStamp(const std::string &timeStampString,
				   timespec &timeStamp,
				   const bool allowEmpty = false);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

typedef UsedCountablePtr<HatoholArmPluginGateHAPI2> HatoholArmPluginGateHAPI2Ptr;

#endif // HatoholArmPluginGateHAPI2_h
