/*
 * Copyright (C) 2014-2015 Project Hatohol
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

#pragma once
#include "FaceRestPrivate.h"

struct RestResourceUser : public RestResourceMemberHandler
{
	typedef void (RestResourceUser::*HandlerFunc)(void);

	static void registerFactories(FaceRest *faceRest);
	static const std::string &getPathForUserMe(void);

	RestResourceUser(FaceRest *faceRest, HandlerFunc handler);
	virtual ~RestResourceUser();

	bool pathIsUserMe(void);

	void handlerUser(void);
	void handlerGetUser(void);
	void handlerPostUser(void);
	void handlerPutUser(void);
	void handlerDeleteUser(void);

	void handlerAccessInfo(void);
	void handlerGetAccessInfo(void);
	void handlerPostAccessInfo(void);
	void handlerDeleteAccessInfo(void);

	void handlerUserRole(void);
	void handlerGetUserRole(void);
	void handlerPostUserRole(void);
	void handlerPutUserRole(void);
	void handlerDeleteUserRole(void);

	/**
	 * Update the user informformation if 'name' specifined in 'query'
	 * exits in the DB. Otherwise, the user is newly added.
	 * NOTE: This method is currently used for test purpose.
	 *
	 * @param query
	 * A hash table that has query parameters in the URL.
	 *
	 * @param option
	 * A UserQueryOption used for the query.
	 *
	 * @return A HatoholError is returned.
	 */
	static HatoholError updateOrAddUser(GHashTable *query,
	                                    UserQueryOption &option);

	static const char *pathForUser;
	static const char *pathForUserRole;
	static const std::string pathForUserMe;
};

