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
#include "config.h"
#include "ConfigManager.h"
#include "Hatohol.h"
using namespace std;;

namespace testConfigManager {

void cut_setup(void)
{
	hatoholInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getActionCommandDirectory(void)
{
	string expect = LIBEXECDIR "/" PACKAGE "/action";
	cppcut_assert_equal(
	  expect, ConfigManager::getInstance()->getActionCommandDirectory());
}

void test_setActionCommandDirectory(void)
{
	string exampleDir = "/usr/hoge/example/dog";
	ConfigManager::getInstance()->setActionCommandDirectory(exampleDir);
	cppcut_assert_equal(
	  exampleDir,
	  ConfigManager::getInstance()->getActionCommandDirectory());
}

void test_getResidentYardDirectory(void)
{
	string expect = PREFIX "/sbin";
	cppcut_assert_equal(
	  expect, ConfigManager::getInstance()->getResidentYardDirectory());
}

void test_setResidentYardDirectory(void)
{
	string exampleDir = "/usr/hoge/example/dog";
	ConfigManager::getInstance()->setResidentYardDirectory(exampleDir);
	cppcut_assert_equal(
	  exampleDir, ConfigManager::getInstance()->getResidentYardDirectory());
}

} // namespace testConfigManager
