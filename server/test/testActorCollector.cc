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
#include <gcutter.h>
#include "ActorCollector.h"
#include "Helpers.h"

using namespace std;
using namespace mlpl;

namespace testActorCollector {

struct TestProfile : public ActorCollector::Profile {
	bool calledSuccessCb;
	bool calledPostSuccessCb;

	TestProfile(void)
	: calledSuccessCb(false),
	  calledPostSuccessCb(false)
	{
	}

	virtual ActorInfo *successCb(void) // override
	{
		calledSuccessCb = true;
		return NULL;
	}

	virtual void postSuccessCb(void) // override
	{
		cppcut_assert_equal(true, calledSuccessCb);
		calledPostSuccessCb = true;
	}

	virtual void errorCb(GError *error) // override
	{
		cut_trace(gcut_assert_error(error));
	}
};

void cut_teardown(void)
{
	ActorCollector::reset();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_debutWithEmptyArgs(void)
{
 	TestProfile profile;
	assertHatoholError(HTERR_INVALID_ARGS, ActorCollector::debut(profile));
}

} // namespace testActorCollector
