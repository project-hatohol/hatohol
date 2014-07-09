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

#ifndef RestResourceAction_h
#define RestResourceAction_h

#include "FaceRestPrivate.h"

struct RestResourceAction : public FaceRest::ResourceHandler
{
	static void registerFactories(FaceRest *faceRest);

	RestResourceAction(FaceRest *faceRest);
	virtual ~RestResourceAction();

	virtual void handle(void) override;

	void handlerAction(void);
	void handlerGetAction(void);
	void handlerPostAction(void);
	void handlerDeleteAction(void);

	static const char *pathForAction;
};

struct RestResourceActionFactory : public FaceRest::ResourceHandlerFactory
{
	RestResourceActionFactory(FaceRest *faceRest);
	virtual FaceRest::ResourceHandler *createHandler(void) override;
};

#endif // RestResourceAction_h
