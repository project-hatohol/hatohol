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
	typedef void (RestResourceUser::*HandlerFunc)(void);

	static void registerFactories(FaceRest *faceRest);

	RestResourceUser(FaceRest *faceRest, HandlerFunc handler);
	virtual ~RestResourceUser();

	virtual void handle(void) override;

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

	static const char *pathForUser;
	static const char *pathForUserRole;

	HandlerFunc m_handlerFunc;
};

struct RestResourceUserFactory : public FaceRest::ResourceHandlerFactory
{
	RestResourceUserFactory(FaceRest *faceRest,
				RestResourceUser::HandlerFunc handler);
	virtual FaceRest::ResourceHandler *createHandler(void) override;

	RestResourceUser::HandlerFunc m_handlerFunc;
};

#endif // RestResourceUser_h
