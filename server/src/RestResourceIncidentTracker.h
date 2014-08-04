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

#ifndef RestResourceIncidentTracker_h
#define RestResourceIncidentTracker_h

#include "FaceRestPrivate.h"

struct RestResourceIncidentTracker : public FaceRest::ResourceHandler
{
	static void registerFactories(FaceRest *faceRest);

	RestResourceIncidentTracker(FaceRest *faceRest);
	virtual ~RestResourceIncidentTracker();

	virtual void handle(void) override;

	void handleGet(void);
	void handlePut(void);
	void handlePost(void);
	void handleDelete(void);

	static const char *pathForIncidentTracker;
};

struct RestResourceIncidentTrackerFactory
: public FaceRest::ResourceHandlerFactory
{
	RestResourceIncidentTrackerFactory(FaceRest *faceRest);
	virtual FaceRest::ResourceHandler *createHandler(void) override;
};

#endif // RestResourceIncidentTracker_h
