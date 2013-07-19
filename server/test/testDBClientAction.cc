/*
 * Copyright (C) 2013 Project Hatohol
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

#include <cppcutter.h>
#include <cutter.h>
#include "Hatohol.h"
#include "Params.h"
#include "DBClientAction.h"
#include "Helpers.h"

namespace testDBClientAction {

void setup(void)
{
	hatoholInit();
	setupTestDBAction();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_dbDomainId(void)
{
	DBClientAction dbAction;
	cppcut_assert_equal(DB_DOMAIN_ID_ACTION,
	                    dbAction.getDBAgent()->getDBDomainId());
}

} // namespace testDBClientAction
