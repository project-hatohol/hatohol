/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#ifndef DBTablesConfig_h
#define DBTablesConfig_h

#include "DBTables.h"
#include "DBTablesMonitoring.h"
#include "MonitoringServerInfo.h"
#include "ArmPluginInfo.h"

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

struct ServerTypeInfo {
	MonitoringSystemType type;
	std::string          name;
	std::string          parameters;
	std::string          pluginPath;
	int                  pluginSQLVersion;
	bool                 pluginEnabled;
	std::string          uuid;
};

typedef std::vector<ServerTypeInfo>        ServerTypeInfoVect;
typedef ServerTypeInfoVect::iterator       ServerTypeInfoVectIterator;
typedef ServerTypeInfoVect::const_iterator ServerTypeInfoVectConstIterator;

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
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class IncidentTrackerQueryOption : public DataQueryOption {
public:
	IncidentTrackerQueryOption(const UserIdType &userId = INVALID_USER_ID);
	IncidentTrackerQueryOption(DataQueryContext *dataQueryContext);
	virtual ~IncidentTrackerQueryOption();

	void setTargetId(const IncidentTrackerIdType &targetId);

	virtual std::string getCondition(void) const; //overrride

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class DBTablesConfig : public DBTables {
public:
	static int CONFIG_DB_VERSION;
	static void reset(void);
	static bool isHatoholArmPlugin(const MonitoringSystemType &type);
	static const SetupInfo &getConstSetupInfo(void);

	DBTablesConfig(DBAgent &dbAgent);
	virtual ~DBTablesConfig();

	std::string getDatabaseDir(void);
	void setDatabaseDir(const std::string &dir);
	int  getFaceRestPort(void);
	void setFaceRestPort(int port);
	bool isCopyOnDemandEnabled(void);

	/**
	 * Register the server type.
	 *
	 * @param serverType
	 * A data to be saved. If serverType.type already exists in the DB,
	 * the record is updated.
	 */
	void registerServerType(const ServerTypeInfo &serverType);

	static std::string getDefaultPluginPath(
	  const MonitoringSystemType &type);

	/**
	 * Get the registered ServerTypeInfo.
	 *
	 * @param serverTypesVect
	 * A obtained records are appended to this object.
	 */
	void getServerTypes(ServerTypeInfoVect &serverTypes);

	/**
	 * Get the registered ServerTypeInfo with the specified type.
	 *
	 * @param serverTypes The obtained information is stored to this.
	 * @param type        The monitoring system type.
	 * @return
	 * true if the server type is found. Otherwise false.
	 */
	bool getServerType(ServerTypeInfo &serverType,
	                   const MonitoringSystemType &type);

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
	static SetupInfo &getSetupInfo(void);
	static void tableInitializerSystem(DBAgent &dbAgent, void *data);
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
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // DBTablesConfig_h
