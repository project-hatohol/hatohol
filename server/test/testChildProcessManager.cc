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

#include <cppcutter.h>
#include "ChildProcessManager.h"
#include "HatoholError.h"
#include "Helpers.h"

namespace testChildProcessManager {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createWithEmptyParameter(void)
{
	ChildProcessManager::CreateArg arg;
	assertHatoholError(HTERR_INVALID_ARGS,
	                   ChildProcessManager::getInstance()->create(arg));
}

} // namespace testChildProcessManager
