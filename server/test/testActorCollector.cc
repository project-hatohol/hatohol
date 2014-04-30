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
#include "ChildProcessManager.h"
#include "Hatohol.h"
#include "Helpers.h"

using namespace std;
using namespace mlpl;

namespace testActorCollector {

struct TestProfile : public ActorCollector::Profile {
	bool calledSuccessCb;
	bool calledPostSuccessCb;
	gint expectError;

	TestProfile(void)
	: calledSuccessCb(false),
	  calledPostSuccessCb(false),
	  expectError(0)
	{
	}

	virtual ActorInfo *successCb(const pid_t &pid) // override
	{
		ActorInfo *actorInfo  = new ActorInfo();
		actorInfo->pid = pid;
		calledSuccessCb = true;
		return actorInfo;
	}

	virtual void postSuccessCb(void) // override
	{
		cppcut_assert_equal(true, calledSuccessCb);
		calledPostSuccessCb = true;
	}

	virtual void errorCb(GError *error) // override
	{
		if (expectError) {
			cppcut_assert_equal(expectError, error->code);
			return;
		}
		cut_trace(gcut_assert_error(error));
	}
};

void cut_setup(void)
{
	hatoholInit();
}

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

void test_debut(void)
{
	TestProfile profile;
	profile.args.push_back("/bin/cat");
	profile.args.push_back("/dev/stdout");
	assertHatoholError(HTERR_OK, ActorCollector::debut(profile));
	cppcut_assert_equal(true, profile.calledPostSuccessCb);
}

void test_debutWithInvalidPath(void)
{
	TestProfile profile;
	profile.args.push_back("non-existing-path");
	profile.expectError = (gint)G_SPAWN_ERROR_NOENT;
	assertHatoholError(HTERR_FAILED_TO_SPAWN,
	                   ActorCollector::debut(profile));
}

} // namespace testActorCollector
