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

#include <cppcutter.h>
#include "Hatohol.h"
#include "Helpers.h"
#include "DBTablesTest.h"
#include "RestResourceTriggerBriefs.h"
#include "FaceRestTestUtils.h"
#include <ThreadLocalDBCache.h>

using namespace std;
using namespace mlpl;

namespace testFaceRestTriggerBriefs {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBTablesUser();
}

void cut_teardown(void)
{
	stopFaceRest();
}

void test_triggerBriefsWithEmptyTriggers(void)
{
	startFaceRest();

        RequestArg arg("/trigger/briefs");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	getServerResponse(arg);
	string expected(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"briefs\":[]"
	  "}");
	assertEqualJSONString(expected, arg.response);
}
}
