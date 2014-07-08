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

#ifndef RestResourceServer_h
#define RestResourceServer_h

#include "FaceRestPrivate.h"

struct RestResourceServer : public FaceRest::ResourceHandler
{
	static void registerFactories(FaceRest *faceRest);

	RestResourceServer(FaceRest *faceRest, RestHandler handler);
	virtual ~RestResourceServer();

	static void handlerServer(ResourceHandler *job);
	static void handlerGetServer(ResourceHandler *job);
	static void handlerPostServer(ResourceHandler *job);
	static void handlerPutServer(ResourceHandler *job);
	static void handlerDeleteServer(ResourceHandler *job);
	static void handlerServerConnStat(ResourceHandler *job);

	static const char *pathForServer;
	static const char *pathForServerConnStat;
};

struct RestResourceServerFactory : public FaceRest::ResourceHandlerFactory
{
	RestResourceServerFactory(FaceRest *faceRest, RestHandler handler);
	virtual FaceRest::ResourceHandler *createHandler(void) override;
};

#endif // RestResourceServer_h
