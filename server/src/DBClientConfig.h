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

#ifndef DBClientConfig_h
#define DBClientConfig_h

#include "DBClient.h"
#include "DBClientHatohol.h"
#include "MonitoringServerInfo.h"

enum IncidentTrackerType {
	INCIDENT_TRACKER_UNKNOWN = -2,
	INCIDENT_TRACKER_FAKE    = -1,
	INCIDENT_TRACKER_REDMINE,
	NUM_INCIDENT_TRACKERS,
};

struct IncidentTrackerInfo {
	IncidentTrackerIdType id;
	IncidentTrackerType   type;
	std::string nickname;
	std::string baseURL;
	std::string projectId;
	std::string trackerId;
	std::string userName;
	std::string password;
};

typedef std::vector<IncidentTrackerInfo>        IncidentTrackerInfoVect;
typedef IncidentTrackerInfoVect::iterator       IncidentTrackerInfoVectIterator;
typedef IncidentTrackerInfoVect::const_iterator IncidentTrackerInfoVectConstIterator;

typedef std::set<IncidentTrackerIdType>         IncidentTrackerIdSet;

struct ArmPluginInfo {
	int id;
	MonitoringSystemType type;
	std::string path;

	/**
	 * The broker URL such as "localhost:5672".
	 * If this value is empty, the default URL is used.
	 */
	std::string brokerUrl;

	/**
	 * If this parameter is empty, dynamically generated queue addresss
	 * is passed to the created plugin proess as an environment variable.
	 * If the plugin process is a passive type (not created by the
	 * Hatohol), the parameter should be set.
	 */
	std::string staticQueueAddress;

	/**
	 * Monitoring server ID of the server this ArmPlugin communicates with.
	 */
	ServerIdType serverId;

	static void initialize(ArmPluginInfo &armPluginInfo);
};

typedef std::vector<ArmPluginInfo>        ArmPluginInfoVect;
typedef ArmPluginInfoVect::iterator       ArmPluginInfoVectIterator;
typedef ArmPluginInfoVect::const_iterator ArmPluginInfoVectConstIterator;

typedef std::map<MonitoringSystemType, ArmPluginInfo *> ArmPluginInfoMap;
typedef ArmPluginInfoMap::iterator          ArmPluginInfoMapIterator;
typedef ArmPluginInfoMap::const_iterator    ArmPluginInfoMapConstIterator;

class ServerQueryOption : public DataQueryOption {
public:
	ServerQueryOption(const UserIdType &userId = INVALID_USER_ID);
	ServerQueryOption(DataQueryContext *dataQueryContext);
	virtual ~ServerQueryOption();

	void setTargetServerId(const ServerIdType &serverId);

	virtual std::string getCondition(void) const override;

protected:
	bool hasPrivilegeCondition(std::string &condition) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

class IncidentTrackerQueryOption : public DataQueryOption {
public:
	IncidentTrackerQueryOption(const UserIdType &userId = INVALID_USER_ID);
	IncidentTrackerQueryOption(DataQueryContext *dataQueryContext);
	virtual ~IncidentTrackerQueryOption();

	void setTargetId(const IncidentTrackerIdType &targetId);

	virtual std::string getCondition(void) const; //overrride

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

class DBClientConfig : public DBClient {
public:
	static int CONFIG_DB_VERSION;
	static const char *DEFAULT_DB_NAME;
	static const char *DEFAULT_USER_NAME;
	static const char *DEFAULT_PASSWORD;
	static void init(const CommandLineArg &cmdArg);
	static void reset(void);
	static bool isHatoholArmPlugin(const MonitoringSystemType &type);

	DBClientConfig(void);
	virtual ~DBClientConfig();

	std::string getDatabaseDir(void);
	void setDatabaseDir(const std::string &dir);
	int  getFaceRestPort(void);
	void setFaceRestPort(int port);
	bool isCopyOnDemandEnabled(void);

	HatoholError addTargetServer(
	  MonitoringServerInfo *monitoringServerInfo,
	  const OperationPrivilege &privilege,
	  ArmPluginInfo *armPluginInfo = NULL);

	HatoholError updateTargetServer(
	  MonitoringServerInfo *monitoringServerInfo,
	  const OperationPrivilege &privilege,
	  ArmPluginInfo *armPluginInfo = NULL);

	HatoholError deleteTargetServer(const ServerIdType &serverId,
	                                const OperationPrivilege &privilege);
	void getTargetServers(MonitoringServerInfoList &monitoringServers,
	                      ServerQueryOption &option,
	                      ArmPluginInfoVect *armPluginInfoVect = NULL);

	/**
	 * Get the ID set of accessible servers.
	 *
	 * @param serverIdSet
	 * The obtained IDs are inserted to this object.
	 * @param dataQueryContext A DataQueryContext instance.
	 */
	void getServerIdSet(ServerIdSet &serverIdSet,
	                    DataQueryContext *dataQueryContext);

	/**
	 * Get all entries in the arm_plugins table.
	 *
	 * @param armPluginVect The obtained data is added to this variable.
	 */
	void getArmPluginInfo(ArmPluginInfoVect &armPluginVect);

	/**
	 * Get ArmPluginInfo with the specified server ID.
	 *
	 * @param armPluginInfo The obtained data is filled to this variable.
	 * @param
	 * serverId A target server ID.
	 *
	 * @return
	 * true if the ArmPluginInfo is got successfully. Otherwise false.
	 */
	bool getArmPluginInfo(ArmPluginInfo &armPluginInfo,
	                      const ServerIdType &serverId);

	/**
	 * Save Arm plugin information.
	 * If the entry with armPluginInfo.type exists, the record is updated.
	 * Otherwise the new record is created.
	 *
	 * @param armPluginInfo A data to be saved.
	 *
	 * @rerurn A HatoholError insntace.
	 */
	HatoholError saveArmPluginInfo(ArmPluginInfo &armPluginInfo);


	/**
	 * Add incident tracker information.
	 *
	 * @param incidentTrackerInfo A data to be saved.
	 * @param privilege
	 * An OperationPrivilege instance. You should set
	 * OPPRVLG_CREATE_INCIDENT_SETTING to execute this function
	 * successfully.
	 *
	 * @rerurn A HatoholError insntace.
	 */
	HatoholError addIncidentTracker(
	  IncidentTrackerInfo &incidentTrackerInfo,
	  const OperationPrivilege &privilege);

	/**
	 * Update an incident tracker information.
	 *
	 * @param incidentTrackerInfo A data to be updated.
	 * @param privilege
	 * An OperationPrivilege instance. You should set
	 * OPPRVLG_UPDATE_INCIDENT_SETTING to execute this function
	 * successfully.
	 *
	 * @rerurn A HatoholError insntace.
	 */
	HatoholError updateIncidentTracker(
	  IncidentTrackerInfo &incidentTrackerInfo,
	  const OperationPrivilege &privilege);

	/**
	 * Delete an incident tracker information.
	 *
	 * @param incidentTrackerInfo A data to be deleted.
	 * @param privilege
	 * An OperationPrivilege instance. You should set
	 * OPPRVLG_DELETE_INCIDENT_SETTING to execute this function
	 * successfully.
	 *
	 * @rerurn A HatoholError insntace.
	 */
	HatoholError deleteIncidentTracker(
	  const IncidentTrackerIdType &incidentTrackerId,
	  const OperationPrivilege &privilege);

	/**
	 * Get entries in the incident_trackers table.
	 *
	 * @param incidentTrackerVect
	 * The obtained data is added to this variable.
	 * @param option
	 * Options for the query
	 */
	void getIncidentTrackers(IncidentTrackerInfoVect &incidentTrackerVect,
				 IncidentTrackerQueryOption &option);

	void getIncidentTrackerIdSet(
	  IncidentTrackerIdSet &incidentTrackerIdSet);

protected:
	static bool parseCommandLineArgument(const CommandLineArg &cmdArg);
	static void tableInitializerSystem(DBAgent *dbAgent, void *data);
	static bool parseDBServer(const std::string &dbServer,
	                          std::string &host, size_t &port);

	static bool canUpdateTargetServer(
	  MonitoringServerInfo *monitoringServerInfo,
	  const OperationPrivilege &privilege);

	static bool canDeleteTargetServer(
	  const ServerIdType &serverId, const OperationPrivilege &privilege);

	void selectArmPluginInfo(DBAgent::SelectExArg &arg);

	void readArmPluginStream(ItemGroupStream &itemGroupStream,
	                         ArmPluginInfo &armPluginInfo);

	HatoholError preprocForSaveArmPlguinInfo(
	  const MonitoringServerInfo &serverInfo,
	  const ArmPluginInfo &armPluginInfo, std::string &condition);
	HatoholError preprocForSaveArmPlguinInfo(
	  const ArmPluginInfo &armPluginInfo, std::string &condition);
	HatoholError saveArmPluginInfoIfNeededWithoutTransaction(
	  ArmPluginInfo &armPluginInfo, const std::string &condition);
	HatoholError saveArmPluginInfoWithoutTransaction(
	  ArmPluginInfo &armPluginInfo, const std::string &condition);

	void preprocForDeleteArmPluginInfo(
	  const ServerIdType &serverId, std::string &condition);
	void deleteArmPluginInfoWithoutTransaction(const std::string &condition);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

};

#endif // DBClientConfig_h
