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

void cut_teardown(void)
{
	ConfigManager::clearParseCommandLineResult();
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

void test_parseConfigServerDefault(void)
{
	ConfigManager *confMgr = ConfigManager::getInstance();
	cppcut_assert_equal(string("localhost"), confMgr->getDBServerAddress());
	cppcut_assert_equal(0, confMgr->getDBServerPort());
}

void test_parseConfigServer(void)
{
	vector<const char *> args;
	args.push_back("command-name");
	args.push_back("--config-db-server");
	args.push_back("umi.example.com");
	gchar **argv = (gchar **)&args[0];
	gint argc = args.size();
	ConfigManager::parseCommandLine(&argc, &argv);
	ConfigManager::reset();
	ConfigManager *confMgr = ConfigManager::getInstance();
	cppcut_assert_equal(string("umi.example.com"),
	                    confMgr->getDBServerAddress());
	cppcut_assert_equal(0, confMgr->getDBServerPort());
}

void test_parseConfigServerWithPort(void)
{
	vector<const char *> args;
	args.push_back("command-name");
	args.push_back("--config-db-server");
	args.push_back("umi.example.com:3333");
	gchar **argv = (gchar **)&args[0];
	gint argc = args.size();
	ConfigManager::parseCommandLine(&argc, &argv);
	ConfigManager::reset();
	ConfigManager *confMgr = ConfigManager::getInstance();
	cppcut_assert_equal(string("umi.example.com"),
	                    confMgr->getDBServerAddress());
	cppcut_assert_equal(3333, confMgr->getDBServerPort());
}

void test_parseTestModeDefault(void)
{
	cppcut_assert_equal(false, ConfigManager::getInstance()->isTestMode());
}

} // namespace testConfigManager
