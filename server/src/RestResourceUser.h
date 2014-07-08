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

#ifndef RestResourceUser_h
#define RestResourceUser_h

#include "FaceRestPrivate.h"

struct RestResourceUser : public FaceRest::ResourceHandler
{
	static void registerFactories(FaceRest *faceRest);

	RestResourceUser(FaceRest *_faceRest, RestHandler _handler);
	virtual ~RestResourceUser();

	static void handlerUser(ResourceHandler *job);
	static void handlerGetUser(ResourceHandler *job);
	static void handlerPostUser(ResourceHandler *job);
	static void handlerPutUser(ResourceHandler *job);
	static void handlerDeleteUser(ResourceHandler *job);

	static void handlerAccessInfo(ResourceHandler *job);
	static void handlerGetAccessInfo(ResourceHandler *job);
	static void handlerPostAccessInfo(ResourceHandler *job);
	static void handlerDeleteAccessInfo(ResourceHandler *job);

	static void handlerUserRole(ResourceHandler *job);
	static void handlerGetUserRole(ResourceHandler *job);
	static void handlerPostUserRole(ResourceHandler *job);
	static void handlerPutUserRole(ResourceHandler *job);
	static void handlerDeleteUserRole(ResourceHandler *job);

	static const char *pathForUser;
	static const char *pathForUserRole;
};

struct RestResourceUserFactory : public FaceRest::ResourceHandlerFactory
{
	RestResourceUserFactory(FaceRest *faceRest, RestHandler handler);
	virtual FaceRest::ResourceHandler *createHandler(void) override;
};

#endif // RestResourceUser_h
