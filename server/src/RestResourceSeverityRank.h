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

#ifndef RestResourceSeverityRank_h
#define RestResourceSeverityRank_h

#include "FaceRestPrivate.h"

struct RestResourceSeverityRank : public FaceRest::ResourceHandler
{
	static void registerFactories(FaceRest *faceRest);

	RestResourceSeverityRank(FaceRest *faceRest);
	virtual ~RestResourceSeverityRank();

	virtual void handle(void) override;

	void handleGet(void);
	void handlePut(void);
	void handlePost(void);
	void handleDelete(void);

	static const char *pathForSeverityRank;
};

#endif // RestResourceSeverityRank_h
