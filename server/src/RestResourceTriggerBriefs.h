/*
 * Copyright (C) 2016 Project Hatohol
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

#ifndef RestResourceTriggerBriefs_h
#define RestResourceTriggerBriefs_h

#include "FaceRestPrivate.h"

struct RestResourceTriggerBriefs : public RestResourceMemberHandler
{
public:
	typedef void (RestResourceTriggerBriefs::*HandlerFunc)(void);

	static const char *pathForTriggerBriefs;

	static void registerFactories(FaceRest *faceRest);

	RestResourceTriggerBriefs(FaceRest *faceRest, HandlerFunc handler);
	void handlerGetTriggerBriefs(void);
};

#endif // RestResourceTriggerBriefs_h
