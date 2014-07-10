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
	typedef void (RestResourceServer::*HandlerFunc)(void);

	static void registerFactories(FaceRest *faceRest);

	RestResourceServer(FaceRest *faceRest, HandlerFunc handler);
	virtual ~RestResourceServer();

	virtual void handle(void) override;

	void handlerServer(void);
	void handlerGetServer(void);
	void handlerPostServer(void);
	void handlerPutServer(void);
	void handlerDeleteServer(void);
	void handlerServerConnStat(void);

	static const char *pathForServer;
	static const char *pathForServerConnStat;

	HandlerFunc m_handlerFunc;
};

struct RestResourceServerFactory : public FaceRest::ResourceHandlerFactory
{
	RestResourceServerFactory(FaceRest *faceRest,
				  RestResourceServer::HandlerFunc handler);
	virtual FaceRest::ResourceHandler *createHandler(void) override;

	RestResourceServer::HandlerFunc m_handlerFunc;
};

#endif // RestResourceServer_h
