/*
 * Copyright (C) 2015 Project Hatohol
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

struct RestResourceSummary : public RestResourceMemberHandler
{
public:
	typedef void (RestResourceSummary::*HandlerFunc)(void);

	static const char *pathForImportantEventSummary;

	static void registerFactories(FaceRest *faceRest);

	RestResourceSummary(FaceRest *faceRest, HandlerFunc handler);
	void handlerImportantEventSummary(void);
};

