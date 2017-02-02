/*
 * Copyright (C) 2013-2015 Project Hatohol
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

#include <stdint.h>
#include "DBTablesUser.h"
#include "DBTablesConfig.h"
#include "ItemGroupStream.h"
#include "DBHatohol.h"
#include "DBTermCStringProvider.h"
using namespace std;
using namespace mlpl;

const UserIdSet EMPTY_USER_ID_SET;
const AccessInfoIdSet EMPTY_ACCESS_INFO_ID_SET;
const UserRoleIdSet EMPTY_USER_ROLE_ID_SET;

// 1 -> 2:
//   * NUM_OPPRVLG:10 -> 16
// 2 -> 3:
//   * NUM_OPPRVLG:16 -> 19
//   * Add user_roles table
// 3 -> 4:
//   * NUM_OPPRVLG:19 -> 23

// -> 1.0
//   * access_list.host_group_id -> VARCHAR
// 1.0 -> 1.1
//   * NUM_OPPRVLG:23 -> 29
//   * Included GET SYSTEM_INFO & SYSTEM_OPERATION privileges migration
// 1.1 -> 1.2
//   * NUM_OPPRVLG:29 -> 32 (Add OPPRVLG_*_CUSTOM_INCIDENT_STATUS)
// 1.2 -> 1.3
//   * NUM_OPPRVLG:32 -> 29
//     Remove OPPRVLG_*_CUSTOM_INCIDENT_STATUS, use OPPRVLG_*_INCIDENT_SETTING
//     instead.

const int DBTablesUser::USER_DB_VERSION =
  DBTables::Version::getPackedVer(0, 1, 3);

const char *DBTablesUser::TABLE_NAME_USERS = "users";
const char *DBTablesUser::TABLE_NAME_ACCESS_LIST = "access_list";
const char *DBTablesUser::TABLE_NAME_USER_ROLES = "user_roles";
const size_t DBTablesUser::MAX_USER_NAME_LENGTH = 128;
const size_t DBTablesUser::MAX_PASSWORD_LENGTH = 128;
const size_t DBTablesUser::MAX_USER_ROLE_NAME_LENGTH = 128;

static const ColumnDef COLUMN_DEF_USERS[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"password",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"flags",                           // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_USERS_ID,
	IDX_USERS_NAME,
	IDX_USERS_PASSWORD,
	IDX_USERS_FLAGS,
	NUM_IDX_USERS,
};

static const DBAgent::TableProfile tableProfileUsers =
  DBAGENT_TABLEPROFILE_INIT(DBTablesUser::TABLE_NAME_USERS,
			    COLUMN_DEF_USERS,
			    NUM_IDX_USERS);

static const ColumnDef COLUMN_DEF_ACCESS_LIST[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"user_id",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_ACCESS_LIST_ID,
	IDX_ACCESS_LIST_USER_ID,
	IDX_ACCESS_LIST_SERVER_ID,
	IDX_ACCESS_LIST_HOST_GROUP_ID,
	NUM_IDX_ACCESS_LIST,
};

static const DBAgent::TableProfile tableProfileAccessList =
  DBAGENT_TABLEPROFILE_INIT(DBTablesUser::TABLE_NAME_ACCESS_LIST,
			    COLUMN_DEF_ACCESS_LIST,
			    NUM_IDX_ACCESS_LIST);

static const ColumnDef COLUMN_DEF_USER_ROLES[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"flags",                           // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_USER_ROLES_ID,
	IDX_USER_ROLES_NAME,
	IDX_USER_ROLES_FLAGS,
	NUM_IDX_USER_ROLES,
};

static const DBAgent::TableProfile tableProfileUserRoles =
  DBAGENT_TABLEPROFILE_INIT(DBTablesUser::TABLE_NAME_USER_ROLES,
			    COLUMN_DEF_USER_ROLES,
			    NUM_IDX_USER_ROLES);

ServerAccessInfoMap::~ServerAccessInfoMap()
{
	for (ServerAccessInfoMapIterator it = begin(); it != end(); ++it) {
		HostGrpAccessInfoMap *hostGrpAccessInfoMap = it->second;
		HostGrpAccessInfoMapIterator jt = hostGrpAccessInfoMap->begin();
		for (; jt != hostGrpAccessInfoMap->end(); ++jt)
			delete jt->second;
		delete hostGrpAccessInfoMap;
	}
}

struct DBTablesUser::Impl {
	static bool validUsernameChars[UINT8_MAX+1];
};

bool DBTablesUser::Impl::validUsernameChars[UINT8_MAX+1];

static void updateAdminPrivilege(DBAgent &dbAgent,
				 const OperationPrivilegeType old_NUM_OPPRVLG)
{
	static const OperationPrivilegeFlag oldAdminFlags =
		OperationPrivilege::makeFlag(old_NUM_OPPRVLG) - 1;
	DBAgent::UpdateArg arg(tableProfileUsers);
	arg.add(IDX_USERS_FLAGS, OperationPrivilege::ALL_PRIVILEGES);
	arg.condition = StringUtils::sprintf(
	  "%s=%" FMT_OPPRVLG,
	  COLUMN_DEF_USERS[IDX_USERS_FLAGS].columnName, oldAdminFlags);
	dbAgent.update(arg);
}

static bool updateDB(
  DBAgent &dbAgent, const DBTables::Version &oldPackedVer, void *data)
{
	static OperationPrivilegeType old_NUM_OPPRVLG;

	const int &oldVer = oldPackedVer.getPackedVer();
	if (oldVer <= 1) {
		old_NUM_OPPRVLG = static_cast<OperationPrivilegeType>(10);
		updateAdminPrivilege(dbAgent, old_NUM_OPPRVLG);
	}
	if (oldVer <= 2) {
		old_NUM_OPPRVLG = static_cast<OperationPrivilegeType>(16);
		updateAdminPrivilege(dbAgent, old_NUM_OPPRVLG);
	}
	if (oldVer <= 3) {
		old_NUM_OPPRVLG = static_cast<OperationPrivilegeType>(19);
		updateAdminPrivilege(dbAgent, old_NUM_OPPRVLG);
	}
	if (oldVer <= DBTables::Version::getPackedVer(0, 1, 1)) {
		old_NUM_OPPRVLG = static_cast<OperationPrivilegeType>(23);
		updateAdminPrivilege(dbAgent, old_NUM_OPPRVLG);
	}
	if (oldVer <= DBTables::Version::getPackedVer(0, 1, 2)) {
		old_NUM_OPPRVLG = static_cast<OperationPrivilegeType>(29);
		updateAdminPrivilege(dbAgent, old_NUM_OPPRVLG);
	}
	if (oldVer <= DBTables::Version::getPackedVer(0, 1, 2)) {
		old_NUM_OPPRVLG = static_cast<OperationPrivilegeType>(32);
		updateAdminPrivilege(dbAgent, old_NUM_OPPRVLG);
	}
	return true;
}

// ---------------------------------------------------------------------------
// UserQueryOption
// ---------------------------------------------------------------------------
struct UserQueryOption::Impl {
	bool   onlyMyself;
	string targetName;
	OperationPrivilegeFlag targetFlags;
	bool   hasPrivilegesFlags;

	Impl(void)
	: onlyMyself(false),
	  targetFlags(0),
	  hasPrivilegesFlags(false)
	{
	}
};

UserQueryOption::UserQueryOption(UserIdType userId)
: DataQueryOption(userId), m_impl(new Impl())
{
}

UserQueryOption::UserQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext),
  m_impl(new Impl())
{
}

UserQueryOption::~UserQueryOption()
{
}

HatoholError UserQueryOption::setTargetName(const string &name)
{
	HatoholError err = DBTablesUser::isValidUserName(name);
	if (err != HTERR_OK)
		return err;
	m_impl->targetName = name;
	return HatoholError(HTERR_OK);
}

OperationPrivilegeFlag UserQueryOption::getPrivilegesFlag(void) const
{
	return m_impl->targetFlags;
}

void UserQueryOption::setPrivilegesFlag(const OperationPrivilegeFlag flags)
{
	m_impl->targetFlags = flags;
	m_impl->hasPrivilegesFlags = true;
}

void UserQueryOption::unsetPrivilegesFlag(void)
{
	m_impl->hasPrivilegesFlags = false;
}

void UserQueryOption::queryOnlyMyself(void)
{
	m_impl->onlyMyself = true;
}

string UserQueryOption::getCondition(void) const
{
	UserIdType userId = getUserId();
	if (userId == INVALID_USER_ID) {
		MLPL_WARN("INVALID_USER_ID\n");
		return DBHatohol::getAlwaysFalseCondition();
	}

	string condition;

	if (m_impl->hasPrivilegesFlags) {
		string nameCond =
		  StringUtils::sprintf("%s=%" FMT_OPPRVLG,
		                       COLUMN_DEF_USERS[IDX_USERS_FLAGS].columnName,
		                       m_impl->targetFlags);
		condition = nameCond;
	}

	if (!has(OPPRVLG_GET_ALL_USER) || m_impl->onlyMyself) {
		DataQueryOption::addCondition(condition,
		  StringUtils::sprintf("%s=%" FMT_USER_ID,
		  COLUMN_DEF_USERS[IDX_USERS_ID].columnName, userId));
	}

	if (!m_impl->targetName.empty()) {
		// The validity of 'targetName' has been checked in
		// setTargetName().
		DBTermCStringProvider rhs(*getDBTermCodec());
		string nameCond =
		  StringUtils::sprintf("%s=%s",
		    COLUMN_DEF_USERS[IDX_USERS_NAME].columnName,
		    rhs(m_impl->targetName));
		DataQueryOption::addCondition(condition, nameCond);
	}
	return condition;
}

// ---------------------------------------------------------------------------
// AccessInfoQueryOption
// ---------------------------------------------------------------------------
struct AccessInfoQueryOption::Impl {
	UserIdType queryUserId;

	Impl(void)
	: queryUserId(INVALID_USER_ID)
	{
	}
};

AccessInfoQueryOption::AccessInfoQueryOption(UserIdType userId)
: DataQueryOption(userId),
  m_impl(new Impl())
{
}

AccessInfoQueryOption::AccessInfoQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext),
  m_impl(new Impl())
{
}

AccessInfoQueryOption::~AccessInfoQueryOption()
{
}
string AccessInfoQueryOption::getCondition(void) const
{
	UserIdType userId = getUserId();
	if (userId == INVALID_USER_ID) {
		MLPL_WARN("INVALID_USER_ID\n");
		return DBHatohol::getAlwaysFalseCondition();
	}

	if (!has(OPPRVLG_GET_ALL_USER) && getUserId() != m_impl->queryUserId) {
		return DBHatohol::getAlwaysFalseCondition();
	}

	return StringUtils::sprintf("%s=%" FMT_USER_ID,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName,
	  getTargetUserId());
}

void AccessInfoQueryOption::setTargetUserId(UserIdType userId)
{
	m_impl->queryUserId = userId;
}

UserIdType AccessInfoQueryOption::getTargetUserId(void) const
{
	return m_impl->queryUserId;
}

// ---------------------------------------------------------------------------
// UserRoleQueryOption
// ---------------------------------------------------------------------------
struct UserRoleQueryOption::Impl {
	UserRoleIdType targetUserRoleId;

	Impl(void)
	: targetUserRoleId(INVALID_USER_ROLE_ID)
	{
	}
};

UserRoleQueryOption::UserRoleQueryOption(UserIdType userId)
: DataQueryOption(userId),
  m_impl(new Impl())
{
}

UserRoleQueryOption::UserRoleQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext),
  m_impl(new Impl())
{
}

UserRoleQueryOption::~UserRoleQueryOption()
{
}

void UserRoleQueryOption::setTargetUserRoleId(UserRoleIdType userRoleId)
{
	m_impl->targetUserRoleId = userRoleId;
}

UserRoleIdType UserRoleQueryOption::getTargetUserRoleId(void) const
{
	return m_impl->targetUserRoleId;
}

string UserRoleQueryOption::getCondition(void) const
{
	string condition;

	if (m_impl->targetUserRoleId != INVALID_USER_ROLE_ID)
		condition = StringUtils::sprintf("%s=%" FMT_USER_ROLE_ID,
		  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_ID].columnName,
		  m_impl->targetUserRoleId);

	return condition;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBTablesUser::init(void)
{
	// set valid characters for the user name
	for (uint8_t c = 'A'; c <= 'Z'; c++)
		Impl::validUsernameChars[c] = true;
	for (uint8_t c = 'a'; c <= 'z'; c++)
		Impl::validUsernameChars[c] = true;
	for (uint8_t c = '0'; c <= '9'; c++)
		Impl::validUsernameChars[c] = true;
	Impl::validUsernameChars[static_cast<uint8_t>('.')] = true;
	Impl::validUsernameChars[static_cast<uint8_t>('-')] = true;
	Impl::validUsernameChars[static_cast<uint8_t>('_')] = true;
	Impl::validUsernameChars[static_cast<uint8_t>('@')] = true;
}

void DBTablesUser::reset(void)
{
	getSetupInfo().initialized = false;
}

const DBTables::SetupInfo &DBTablesUser::getConstSetupInfo(void)
{
	return getSetupInfo();
}

DBTablesUser::DBTablesUser(DBAgent &dbAgent)
: DBTables(dbAgent, getSetupInfo()),
  m_impl(new Impl())
{
}

DBTablesUser::~DBTablesUser()
{
}

HatoholError DBTablesUser::addUserInfo(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	HatoholError err;
	if (!privilege.has(OPPRVLG_CREATE_USER))
		return HatoholError(HTERR_NO_PRIVILEGE);
	err = isValidUserName(userInfo.name);
	if (err != HTERR_OK)
		return err;
	err = isValidPassword(userInfo.password);
	if (err != HTERR_OK)
		return err;
	err = isValidFlags(userInfo.flags);
	if (err != HTERR_OK)
		return err;

	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError err;
		string dupCheckCond;
		DBAgent::InsertArg arg;
		UserInfo &userInfo;

		TrxProc(UserInfo &_userInfo)
		: arg(tableProfileUsers),
		  userInfo(_userInfo)
		{
		}

		bool preproc(DBAgent &dbAgent) override
		{
			DBTermCStringProvider rhs(*dbAgent.getDBTermCodec());
			arg.add(AUTO_INCREMENT_VALUE);
			arg.add(userInfo.name);
			arg.add(Utils::sha256(userInfo.password));
			arg.add(userInfo.flags);
			dupCheckCond = StringUtils::sprintf("%s=%s",
			  COLUMN_DEF_USERS[IDX_USERS_NAME].columnName,
			  rhs(userInfo.name));
			return true;
		}

		bool hasRecord(DBAgent &dbAgent)
		{
			return dbAgent.isRecordExisting(TABLE_NAME_USERS,
							dupCheckCond);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			if (hasRecord(dbAgent)) {
				err = HTERR_USER_NAME_EXIST;
			} else {
				dbAgent.insert(arg);
				userInfo.id = dbAgent.getLastInsertId();
				err = HTERR_OK;
			}
		}
	} trx(userInfo);
	getDBAgent().runTransaction(trx);
	return trx.err;
}

HatoholError DBTablesUser::hasPrivilegeForUpdateUserInfo(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	if (privilege.has(OPPRVLG_UPDATE_USER))
		return HTERR_OK;
	if (userInfo.id != privilege.getUserId())
		return HTERR_NO_PRIVILEGE;
	if (userInfo.id == INVALID_USER_ID)
		return HTERR_NO_PRIVILEGE;

	UserInfo userInfoInDB;
	bool exist = getUserInfo(userInfoInDB, userInfo.id);
	if (!exist)
		return HTERR_NO_PRIVILEGE;
	userInfoInDB.password = userInfo.password;
	if (userInfoInDB == userInfo)
		return HTERR_OK;

	return HTERR_NO_PRIVILEGE;
}

HatoholError DBTablesUser::updateUserInfo(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	HatoholError err = hasPrivilegeForUpdateUserInfo(userInfo, privilege);
	if (err != HTERR_OK)
		return err;
	err = isValidUserName(userInfo.name);
	if (err != HTERR_OK)
		return err;
	err = isValidPassword(userInfo.password);
	if (!userInfo.password.empty() && err != HTERR_OK)
		return err;
	err = isValidFlags(userInfo.flags);
	if (err != HTERR_OK)
		return err;

	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError err;
		string dupCheckCond;
		DBAgent::UpdateArg arg;
		UserInfo &userInfo;

		TrxProc(UserInfo &_userInfo)
		: arg(tableProfileUsers),
		  userInfo(_userInfo)
		{
			arg.add(IDX_USERS_NAME, userInfo.name);
			if (!userInfo.password.empty()) {
				arg.add(IDX_USERS_PASSWORD,
				        Utils::sha256(userInfo.password));
			}
			arg.add(IDX_USERS_FLAGS, userInfo.flags);

			arg.condition = StringUtils::sprintf("%s=%" FMT_USER_ID,
			  COLUMN_DEF_USERS[IDX_USERS_ID].columnName,
			  userInfo.id);
		}

		bool preproc(DBAgent &dbAgent) override
		{
			DBTermCStringProvider rhs(*dbAgent.getDBTermCodec());
			dupCheckCond = StringUtils::sprintf(
			  "(%s=%s and %s<>%" FMT_USER_ID ")",
			  COLUMN_DEF_USERS[IDX_USERS_NAME].columnName,
			  rhs(userInfo.name),
			  COLUMN_DEF_USERS[IDX_USERS_ID].columnName,
			  userInfo.id);
			return true;
		}

		bool hasRecord(DBAgent &dbAgent, const string &condition)
		{
			return dbAgent.isRecordExisting(arg.tableProfile.name,
							condition);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			if (!hasRecord(dbAgent, arg.condition)) {
				err = HTERR_NOT_FOUND_TARGET_RECORD;
			} else if (hasRecord(dbAgent, dupCheckCond)) {
				err = HTERR_USER_NAME_EXIST;
			} else {
				dbAgent.update(arg);
				err = HTERR_OK;
			}
		}
	} trx(userInfo);
	getDBAgent().runTransaction(trx);
	return trx.err;
}

HatoholError DBTablesUser::updateUserInfoFlags(
  OperationPrivilegeFlag &oldUserFlag, OperationPrivilegeFlag &updateUserFlag,
  const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_UPDATE_USER))
		return HTERR_NO_PRIVILEGE;
	HatoholError err;
	err = isValidFlags(oldUserFlag);
	if (err != HTERR_OK)
		return err;
	err = isValidFlags(updateUserFlag);
	if (err != HTERR_OK)
		return err;

	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError err;
		DBAgent::UpdateArg arg;
		OperationPrivilegeFlag &oldUserFlag, &updateUserFlag;

		TrxProc(OperationPrivilegeFlag &_oldUserFlag, OperationPrivilegeFlag &_updateUserFlag)
		: arg(tableProfileUsers),
		  oldUserFlag(_oldUserFlag),
		  updateUserFlag(_updateUserFlag)
		{
			arg.add(IDX_USERS_FLAGS, updateUserFlag);

			arg.condition = StringUtils::sprintf("%s=%" PRIu64,
			  COLUMN_DEF_USERS[IDX_USERS_FLAGS].columnName,
			  oldUserFlag);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.update(arg);
			err = HTERR_OK;
		}
	} trx(oldUserFlag, updateUserFlag);
	getDBAgent().runTransaction(trx);
	return trx.err;
}

HatoholError DBTablesUser::deleteUserInfo(
  const UserIdType userId, const OperationPrivilege &privilege)
{
	using mlpl::StringUtils::sprintf;
	if (!privilege.has(OPPRVLG_DELETE_USER))
		return HTERR_NO_PRIVILEGE;
	if (userId == privilege.getUserId())
		return HTERR_DELETE_MYSELF;

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg argForUsers;
		DBAgent::DeleteArg argForAccessList;

		TrxProc(const UserIdType &userId)
		: argForUsers(tableProfileUsers),
		 argForAccessList(tableProfileAccessList)
		{
			argForUsers.condition = sprintf("%s=%" FMT_USER_ID,
			  COLUMN_DEF_USERS[IDX_USERS_ID].columnName, userId);
			argForAccessList.condition = sprintf("%s=%" FMT_USER_ID,
			  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName,
			  userId);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(argForUsers);
			dbAgent.deleteRows(argForAccessList);
		}
	} trx(userId);
	getDBAgent().runTransaction(trx);
	return HTERR_OK;
}

UserIdType DBTablesUser::getUserId(const string &user, const string &password)
{
	if (isValidUserName(user) != HTERR_OK)
		return INVALID_USER_ID;
	if (isValidPassword(password) != HTERR_OK)
		return INVALID_USER_ID;

	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::SelectExArg arg(tableProfileUsers);
	arg.add(IDX_USERS_ID);
	arg.add(IDX_USERS_PASSWORD);
	arg.condition = StringUtils::sprintf("%s=%s",
	  COLUMN_DEF_USERS[IDX_USERS_NAME].columnName, rhs(user));
	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	if (grpList.empty())
		return INVALID_USER_ID;

	ItemGroupStream itemGroupStream(*grpList.begin());
	UserIdType userId;
	string truePasswd;
	itemGroupStream >> userId;
	itemGroupStream >> truePasswd;

	// comapare the passwords
	bool matched = (truePasswd == Utils::sha256(password));
	if (!matched)
		return INVALID_USER_ID;
	return userId;
}

HatoholError DBTablesUser::addAccessInfo(AccessInfo &accessInfo,
					 const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_UPDATE_USER))
		return HatoholError(HTERR_NO_PRIVILEGE);

	// check existing data
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::SelectExArg selarg(tableProfileAccessList);
	selarg.add(IDX_ACCESS_LIST_ID);
	selarg.add(IDX_ACCESS_LIST_USER_ID);
	selarg.add(IDX_ACCESS_LIST_SERVER_ID);
	selarg.add(IDX_ACCESS_LIST_HOST_GROUP_ID);
	selarg.condition = StringUtils::sprintf(
	  "%s=%" FMT_USER_ID " AND %s=%" PRIu32 " AND %s=%s",
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName,
	  accessInfo.userId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID].columnName,
	  accessInfo.serverId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID].columnName,
	  rhs(accessInfo.hostgroupId));
	getDBAgent().runTransaction(selarg);

	const ItemGroupList &grpList = selarg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	if (it != grpList.end()) {
		const ItemGroup *itemGroup = *it;
		accessInfo.id = *itemGroup->getItemAt(0);
		return HTERR_OK;
	}

	// add new data
	DBAgent::InsertArg arg(tableProfileAccessList);
	arg.add(AUTO_INCREMENT_VALUE);
	arg.add(accessInfo.userId);
	arg.add(accessInfo.serverId);
	arg.add(accessInfo.hostgroupId);

	getDBAgent().runTransaction(arg, accessInfo.id);
	return HTERR_OK;
}

HatoholError DBTablesUser::deleteAccessInfo(const AccessInfoIdType id,
					    const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_UPDATE_USER))
		return HatoholError(HTERR_NO_PRIVILEGE);

	DBAgent::DeleteArg arg(tableProfileAccessList);
	const ColumnDef &colId = COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_ID];
	arg.condition = StringUtils::sprintf("%s=%" FMT_ACCESS_INFO_ID,
	                                     colId.columnName, id);
	getDBAgent().runTransaction(arg);
	return HTERR_OK;
}

bool DBTablesUser::getUserInfo(UserInfo &userInfo, const UserIdType userId)
{
	UserInfoList userInfoList;
	string condition = StringUtils::sprintf("%s=%" FMT_USER_ID,
	  COLUMN_DEF_USERS[IDX_USERS_ID].columnName, userId);
	getUserInfoList(userInfoList, condition);
	if (userInfoList.empty())
		return false;
	HATOHOL_ASSERT(userInfoList.size() == 1, "userInfoList.size(): %zd\n",
	               userInfoList.size());
	userInfo = *userInfoList.begin();
	return true;
}

void DBTablesUser::getUserIdSet(UserIdSet &userIdSet)
{
	UserInfoList userInfoList;
	const string condition = "";

	getUserInfoList(userInfoList, condition);
	if (userInfoList.empty())
		return;
	UserInfoListIterator it = userInfoList.begin();
	for(; it != userInfoList.end();++it) {
		const UserInfo &userInfo = *it;
		userIdSet.insert(userInfo.id);
	}
}

void DBTablesUser::getUserInfoList(UserInfoList &userInfoList,
                                   const UserQueryOption &option)
{
	getUserInfoList(userInfoList, option.getCondition());
}

HatoholError DBTablesUser::getAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
					    const AccessInfoQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileAccessList);
	arg.add(IDX_ACCESS_LIST_ID);
	arg.add(IDX_ACCESS_LIST_USER_ID);
	arg.add(IDX_ACCESS_LIST_SERVER_ID);
	arg.add(IDX_ACCESS_LIST_HOST_GROUP_ID);
	arg.condition = option.getCondition();

	if (DBHatohol::isAlwaysFalseCondition(arg.condition))
		return HTERR_NO_PRIVILEGE;

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		AccessInfo *accessInfo = new AccessInfo();
		itemGroupStream >> accessInfo->id;
		itemGroupStream >> accessInfo->userId;
		itemGroupStream >> accessInfo->serverId;
		itemGroupStream >> accessInfo->hostgroupId;

		// insert data
		HostGrpAccessInfoMap *hostGrpAccessInfoMap = NULL;
		ServerAccessInfoMapIterator it =
		  srvAccessInfoMap.find(accessInfo->serverId);
		if (it == srvAccessInfoMap.end()) {
			hostGrpAccessInfoMap = new HostGrpAccessInfoMap();
			srvAccessInfoMap[accessInfo->serverId] =
			   hostGrpAccessInfoMap;
		} else {
			hostGrpAccessInfoMap = it->second;
		}

		HostGrpAccessInfoMapIterator jt =
		  hostGrpAccessInfoMap->find(accessInfo->hostgroupId);
		if (jt != hostGrpAccessInfoMap->end()) {
			MLPL_WARN(
			  "Found duplicated serverId and hostgroupId: "
			  "%" FMT_SERVER_ID ", %" FMT_HOST_GROUP_ID "\n",
			   accessInfo->serverId,
			   accessInfo->hostgroupId.c_str());
			delete accessInfo;
			continue;
		}
		(*hostGrpAccessInfoMap)[accessInfo->hostgroupId] = accessInfo;
	}

	return HTERR_OK;
}

void DBTablesUser::destroyServerAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap)
{
	ServerAccessInfoMapIterator it = srvAccessInfoMap.begin();
	for (; it != srvAccessInfoMap.end(); ++it) {
		HostGrpAccessInfoMap *hostGrpAccessInfoMap = it->second;
		HostGrpAccessInfoMapIterator it2 = hostGrpAccessInfoMap->begin();
		for (; it2 != hostGrpAccessInfoMap->end(); it2++) {
			AccessInfo *info = it2->second;
			delete info;
		}
		delete hostGrpAccessInfoMap;
	}
	srvAccessInfoMap.clear();
}

void DBTablesUser::getServerHostGrpSetMap(
  ServerHostGrpSetMap &srvHostGrpSetMap, const UserIdType &userId)
{
	DBAgent::SelectExArg arg(tableProfileAccessList);
	arg.add(IDX_ACCESS_LIST_SERVER_ID);
	arg.add(IDX_ACCESS_LIST_HOST_GROUP_ID);
	arg.condition = StringUtils::sprintf("%s=%" FMT_USER_ID,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName, userId);
	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ServerIdType serverId;
		HostgroupIdType hostgroupId;
		ItemGroupStream itemGroupStream(*itemGrpItr);
		itemGroupStream >> serverId;
		itemGroupStream >> hostgroupId;

		// insert data
		pair<HostgroupIdSetIterator, bool> result =
		  srvHostGrpSetMap[serverId].insert(hostgroupId);
		if (!result.second) {
			MLPL_WARN("Found duplicated serverId and hostgroupId: "
			          "%" FMT_SERVER_ID ", "
			          "%" FMT_HOST_GROUP_ID "\n",
			          serverId, hostgroupId.c_str());
			continue;
		}
	}
}

HatoholError DBTablesUser::addUserRoleInfo(UserRoleInfo &userRoleInfo,
					   const OperationPrivilege &privilege)
{
	HatoholError err;
	if (!privilege.has(OPPRVLG_CREATE_USER_ROLE))
		return HatoholError(HTERR_NO_PRIVILEGE);
	err = isValidUserRoleName(userRoleInfo.name);
	if (err != HTERR_OK)
		return err;
	err = isValidFlags(userRoleInfo.flags);
	if (err != HTERR_OK)
		return err;
	if (userRoleInfo.flags == OperationPrivilege::ALL_PRIVILEGES ||
	    userRoleInfo.flags == OperationPrivilege::NONE_PRIVILEGE) {
		return HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError       err;
		DBAgent::InsertArg arg;
		string             dupChkCond;
		UserRoleInfo      &userRoleInfo;

		TrxProc(UserRoleInfo &_userRoleInfo)
		: arg(tableProfileUserRoles),
		  userRoleInfo(_userRoleInfo)
		{
			arg.add(AUTO_INCREMENT_VALUE);
			arg.add(userRoleInfo.name);
			arg.add(userRoleInfo.flags);
		}

		bool hasRecord(DBAgent &dbAgent)
		{
			return dbAgent.isRecordExisting(TABLE_NAME_USER_ROLES,
							dupChkCond);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			if (hasRecord(dbAgent)) {
				err = HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST;
				return;
			}
			dbAgent.insert(arg);
			userRoleInfo.id = dbAgent.getLastInsertId();
			err = HTERR_OK;
		}
	} trx(userRoleInfo);

	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	trx.dupChkCond = StringUtils::sprintf(
	  "(%s=%s or %s=%" FMT_OPPRVLG ")",
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_NAME].columnName,
	  rhs(userRoleInfo.name),
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_FLAGS].columnName,
	  userRoleInfo.flags);
	getDBAgent().runTransaction(trx);
	return trx.err;
}

HatoholError DBTablesUser::updateUserRoleInfo(
  UserRoleInfo &userRoleInfo, const OperationPrivilege &privilege)
{
	HatoholError err;
	if (!privilege.has(OPPRVLG_UPDATE_ALL_USER_ROLE))
		return HatoholError(HTERR_NO_PRIVILEGE);
	err = isValidUserRoleName(userRoleInfo.name);
	if (err != HTERR_OK)
		return err;
	err = isValidFlags(userRoleInfo.flags);
	if (err != HTERR_OK)
		return err;
	if (userRoleInfo.flags == OperationPrivilege::ALL_PRIVILEGES ||
	    userRoleInfo.flags == OperationPrivilege::NONE_PRIVILEGE) {
		return HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError       err;
		DBAgent::UpdateArg arg;
		string             dupChkCond;

		TrxProc(void)
		: arg(tableProfileUserRoles)
		{
		}

		bool hasRecord(DBAgent &dbAgent, const string &condition)
		{
			return dbAgent.isRecordExisting(arg.tableProfile.name,
							condition);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			if (!hasRecord(dbAgent, arg.condition)) {
				err = HTERR_NOT_FOUND_TARGET_RECORD;
			} else if (hasRecord(dbAgent, dupChkCond)) {
				err = HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST;
			} else {
				dbAgent.update(arg);
				err = HTERR_OK;
			}
		}
	} trx;

	DBAgent::UpdateArg &arg = trx.arg;
	arg.add(IDX_USER_ROLES_NAME, userRoleInfo.name);
	arg.add(IDX_USER_ROLES_FLAGS, userRoleInfo.flags);

	arg.condition = StringUtils::sprintf("%s=%" FMT_USER_ROLE_ID,
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_ID].columnName,
	  userRoleInfo.id);

	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	trx.dupChkCond = StringUtils::sprintf(
	  "((%s=%s or %s=%" FMT_OPPRVLG ") and %s<>%" FMT_USER_ROLE_ID ")",
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_NAME].columnName,
	  rhs(userRoleInfo.name),
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_FLAGS].columnName,
	  userRoleInfo.flags,
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_ID].columnName,
	  userRoleInfo.id);

	getDBAgent().runTransaction(trx);
	return trx.err;
}

HatoholError DBTablesUser::deleteUserRoleInfo(
  const UserRoleIdType userRoleId, const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_DELETE_ALL_USER_ROLE))
		return HTERR_NO_PRIVILEGE;

	DBAgent::DeleteArg arg(tableProfileUserRoles);
	const ColumnDef &colId = COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_ID];
	arg.condition = StringUtils::sprintf("%s=%" FMT_USER_ROLE_ID,
	                                     colId.columnName, userRoleId);
	getDBAgent().runTransaction(arg);
	return HTERR_OK;
}

void DBTablesUser::getUserRoleInfoList(UserRoleInfoList &userRoleInfoList,
				       const UserRoleQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileUserRoles);
	arg.add(IDX_USER_ROLES_ID);
	arg.add(IDX_USER_ROLES_NAME);
	arg.add(IDX_USER_ROLES_FLAGS);
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		UserRoleInfo userRoleInfo;
		itemGroupStream >> userRoleInfo.id;
		itemGroupStream >> userRoleInfo.name;
		itemGroupStream >> userRoleInfo.flags;
		userRoleInfoList.push_back(userRoleInfo);
	}
}

HatoholError DBTablesUser::isValidUserName(const string &name)
{
	if (name.empty())
		return HTERR_EMPTY_USER_NAME;
	if (name.size() > MAX_USER_NAME_LENGTH)
		return HTERR_TOO_LONG_USER_NAME;
	for (const char *p = name.c_str(); *p; p++) {
		uint8_t idx = *p;
		if (!Impl::validUsernameChars[idx])
			return HTERR_INVALID_CHAR;
	}
	return HTERR_OK;
}

HatoholError DBTablesUser::isValidPassword(const string &password)
{
	if (password.empty())
		return HTERR_EMPTY_PASSWORD;
	if (password.size() > MAX_PASSWORD_LENGTH)
		return HTERR_TOO_LONG_PASSWORD;
	return HTERR_OK;
}

HatoholError DBTablesUser::isValidFlags(const OperationPrivilegeFlag flags)
{
	if (flags >= OperationPrivilege::makeFlag(NUM_OPPRVLG))
		return HTERR_INVALID_PRIVILEGE_FLAGS;
	return HTERR_OK;
}

HatoholError DBTablesUser::isValidUserRoleName(const string &name)
{
	if (name.empty())
		return HTERR_EMPTY_USER_ROLE_NAME;
	if (name.size() > MAX_USER_ROLE_NAME_LENGTH)
		return HTERR_TOO_LONG_USER_ROLE_NAME;
	return HTERR_OK;
}

bool DBTablesUser::isAccessible(const ServerIdType &serverId,
                                const OperationPrivilege &privilege,
                                const bool &useTransaction)
{
	UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID) {
		MLPL_WARN("INVALID_USER_ID\n");
		return false;
	}

	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	string condition = StringUtils::sprintf(
	  "%s=%" FMT_USER_ID " AND "
	  "(%s=%" FMT_SERVER_ID " OR %s=%" FMT_SERVER_ID ") AND %s=%s",
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName,
	  userId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID].columnName,
	  serverId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID].columnName,
	  ALL_SERVERS,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID].columnName,
	  rhs(ALL_HOST_GROUPS));

	DBAgent::SelectExArg arg(tableProfileAccessList);
	arg.add("count(*)", SQL_COLUMN_TYPE_INT);
	arg.condition = condition;

	if (useTransaction) {
		getDBAgent().runTransaction(arg);
	} else {
		select(arg);
	}

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "No result");
	const ItemGroup *itemGroup = *grpList.begin();
	int count = *itemGroup->getItemAt(0);
	return count > 0;
}

bool DBTablesUser::isAccessible(
  const ServerIdType &serverId, const HostgroupIdType &hostgroupId,
  const OperationPrivilege &privilege, const bool &useTransaction)
{
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	const UserIdType userId = privilege.getUserId();
	string condition = StringUtils::sprintf(
	  "%s=%" FMT_USER_ID " AND "
	  "((%s=%" FMT_SERVER_ID ") OR "
	  " (%s=%" FMT_SERVER_ID " AND "
	  "  (%s=%s OR %s=%s)))",
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName,
	  userId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID].columnName,
	  ALL_SERVERS,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID].columnName,
	  serverId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID].columnName,
	  rhs(ALL_HOST_GROUPS),
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID].columnName,
	  rhs(hostgroupId));

	return isAccessible(privilege, condition, useTransaction);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
DBTables::SetupInfo &DBTablesUser::getSetupInfo(void)
{
	static const TableSetupInfo DB_TABLE_INFO[] = {
	{
		&tableProfileUsers,
	}, {
		&tableProfileAccessList,
	}, {
		&tableProfileUserRoles,
	},
	};

	static SetupInfo setupInfo = {
		DBTablesId::USER,
		USER_DB_VERSION,
		ARRAY_SIZE(DB_TABLE_INFO),
		DB_TABLE_INFO,
		&updateDB,
	};
	return setupInfo;
}

void DBTablesUser::getUserInfoList(UserInfoList &userInfoList,
                                   const string &condition)
{
	if (DBHatohol::isAlwaysFalseCondition(condition))
		return;

	DBAgent::SelectExArg arg(tableProfileUsers);
	arg.add(IDX_USERS_ID);
	arg.add(IDX_USERS_NAME);
	arg.add(IDX_USERS_PASSWORD);
	arg.add(IDX_USERS_FLAGS);
	arg.condition = condition;

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		UserInfo userInfo;
		itemGroupStream >> userInfo.id;
		itemGroupStream >> userInfo.name;
		itemGroupStream >> userInfo.password;
		itemGroupStream >> userInfo.flags;
		userInfoList.push_back(userInfo);
	}
}

bool DBTablesUser::isAccessible(
  const OperationPrivilege &privilege, const string &condition,
  const bool &useTransaction)
{
	UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID) {
		MLPL_WARN("INVALID_USER_ID\n");
		return false;
	}

	DBAgent::SelectExArg arg(tableProfileAccessList);
	arg.add("count(*)", SQL_COLUMN_TYPE_INT);
	arg.condition = condition;

	if (useTransaction) {
		getDBAgent().runTransaction(arg);
	} else {
		select(arg);
	}

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "No result");
	const ItemGroup *itemGroup = *grpList.begin();
	int count = *itemGroup->getItemAt(0);
	return count > 0;
}
