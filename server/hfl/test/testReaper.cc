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
#include "Reaper.h"

using namespace hfl;

namespace testReaper {

struct TestContext {
	bool called;

	TestContext(void)
	: called(false)
	{
	}
};

class TestContextOperator {
	TestContext *m_ctx;

public:
	TestContextOperator(TestContext *ctx)
	: m_ctx(ctx)
	{
	}

	virtual ~TestContextOperator()
	{
		m_ctx->called = true;
	}
};

static void destFunc(TestContext *ctx)
{
	ctx->called = true;
}

void test_destructFuncCalled(void)
{
	TestContext ctx;
	cppcut_assert_equal(false, ctx.called);
	{
		Reaper<TestContext> var(&ctx, destFunc);
	}
	cppcut_assert_equal(true, ctx.called);
}

void test_destructCppObject(void)
{
	TestContext ctx;
	cppcut_assert_equal(false, ctx.called);
	{
		CppReaper<TestContextOperator>
		   var(new TestContextOperator(&ctx));
	}
	cppcut_assert_equal(true, ctx.called);
}

void test_reap(void)
{
	TestContext ctx;
	cppcut_assert_equal(false, ctx.called);
	Reaper<TestContext> var(&ctx, destFunc);
	var.reap();
	cppcut_assert_equal(true, ctx.called);

	// call twice
	ctx.called = false;
	var.reap();
	cppcut_assert_equal(false, ctx.called);
}

void test_reapCppObject(void)
{
	TestContext ctx;
	cppcut_assert_equal(false, ctx.called);
	CppReaper<TestContextOperator> var(new TestContextOperator(&ctx));
	var.reap();
	cppcut_assert_equal(true, ctx.called);

	// call twice
	ctx.called = false;
	var.reap();
	cppcut_assert_equal(false, ctx.called);
}

void test_deactivate(void)
{
	TestContext ctx;
	cppcut_assert_equal(false, ctx.called);
	{
		Reaper<TestContext> var(&ctx, destFunc);
		var.deactivate();
	}
	cppcut_assert_equal(false, ctx.called);
}

void test_set(void)
{
	TestContext ctx;
	cppcut_assert_equal(false, ctx.called);
	{
		Reaper<TestContext> var;
		cppcut_assert_equal(true, var.set(&ctx, destFunc));
	}
	cppcut_assert_equal(true, ctx.called);
}

void test_setDouble(void)
{
	TestContext ctx;
	cppcut_assert_equal(false, ctx.called);
	{
		Reaper<TestContext> var;
		cppcut_assert_equal(true, var.set(&ctx, destFunc));
		cppcut_assert_equal(false, var.set(&ctx, destFunc));
	}
	cppcut_assert_equal(true, ctx.called);
}

void test_setCpp(void)
{
	TestContext ctx;
	cppcut_assert_equal(false, ctx.called);
	{
		CppReaper<TestContextOperator> var;
		cppcut_assert_equal(true,
		                    var.set(new TestContextOperator(&ctx)));
	}
	cppcut_assert_equal(true, ctx.called);
}

void test_get(void)
{
	void *ptr = malloc(10);
	Reaper<void> reaper(ptr, free);
	cppcut_assert_equal(ptr, reaper.get());
}

} // namespace testReaper
