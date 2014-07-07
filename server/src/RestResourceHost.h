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

#ifndef RestResourceHost_h
#define RestResourceHost_h

#include "FaceRestPrivate.h"

struct RestResourceHost : public FaceRest::ResourceHandler
{
	static void handlerGetOverview(ResourceHandler *job);
	static void handlerGetHost(ResourceHandler *job);
	static void handlerGetTrigger(ResourceHandler *job);
	static void handlerGetEvent(ResourceHandler *job);
	static void handlerGetHostgroup(ResourceHandler *job);
	static void handlerGetItem(ResourceHandler *job);
	static void replyGetItem(ResourceHandler *job);

	static HatoholError parseEventParameter(EventsQueryOption &option,
						GHashTable *query);

	static const char *pathForOverview;
	static const char *pathForHost;
	static const char *pathForTrigger;
	static const char *pathForEvent;
	static const char *pathForItem;
	static const char *pathForHostgroup;
};

#endif // RestResourceHost_h
