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

#include <cppcutter.h>
#include "Hatohol.h"
#include "Helpers.h"
#include "DBTablesTest.h"
#include "RestResourceSystem.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testFaceRestSystem {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBUser();
}

void cut_teardown(void)
{
	stopFaceRest();
}

void test_systemInfo(void)
{
	startFaceRest();
	RequestArg arg("/system-info");
	arg.userId = findUserWith(OPPRVLG_GET_SYSTEM_INFO);
	unique_ptr<JSONParser> parserPtr(getResponseAsJSONParser(arg));
	JSONParser *parser = parserPtr.get();
	assertErrorCode(parser);

	auto assertTime = [&](const char *label) {
		string s;
		cppcut_assert_equal(true, parser->read(label, s));
		cppcut_assert_equal(false, s.empty());
		cppcut_assert_equal(true, StringUtils::isNumber(s));
	};

	auto assertNumber = [&]() {
		int64_t n;
		cppcut_assert_equal(true, parser->read("number", n));
	};

	assertStartObject(parser, "eventRates");
	for (size_t i = 0; i < DBTablesMonitoring::NUM_EVENTS_COUNTERS; i++) {
		parser->startElement(i);
		for (auto label : {"previous", "current"}) {
			assertStartObject(parser, label);
			for (auto label : {"startTime", "endTime"})
				assertTime(label);
			assertNumber();
			parser->endObject();
		}
		parser->endElement();
	}
}

} // namespace testFaceRestSystem
