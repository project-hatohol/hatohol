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
#include "Helpers.h"
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

void test_parseConfigServerDefault(void)
{
	ConfigManager *confMgr = ConfigManager::getInstance();
	cppcut_assert_equal(string("localhost"), confMgr->getDBServerAddress());
	cppcut_assert_equal(0, confMgr->getDBServerPort());
}

void test_parseConfigServer(void)
{
	CommandArgHelper cmds;
	cmds << "--config-db-server";
	cmds << "umi.example.com";
	cmds.activate();
	ConfigManager *confMgr = ConfigManager::getInstance();
	cppcut_assert_equal(string("umi.example.com"),
	                    confMgr->getDBServerAddress());
	cppcut_assert_equal(0, confMgr->getDBServerPort());
}

void test_parseConfigServerWithPort(void)
{
	CommandArgHelper cmds;
	cmds << "--config-db-server";
	cmds << "umi.example.com:3333";
	cmds.activate();
	ConfigManager *confMgr = ConfigManager::getInstance();
	cppcut_assert_equal(string("umi.example.com"),
	                    confMgr->getDBServerAddress());
	cppcut_assert_equal(3333, confMgr->getDBServerPort());
}

void test_parseTestModeDefault(void)
{
	cppcut_assert_equal(false, ConfigManager::getInstance()->isTestMode());
}

void test_parseTestModeEnabled(void)
{
	CommandArgHelper cmds;
	cmds << "--test-mode";
	cmds.activate();
	cppcut_assert_equal(true, ConfigManager::getInstance()->isTestMode());
}

void test_parseCopyOnDemandDefault(void)
{
	cppcut_assert_equal(
	  ConfigManager::UNKNOWN,
	  ConfigManager::getInstance()->getCopyOnDemand());
}

void test_parseEnableCopyOnDemand(void)
{
	CommandArgHelper cmds;
	cmds << "--enable-copy-on-demand";
	cmds.activate();
	cppcut_assert_equal(
	  ConfigManager::ENABLE,
	  ConfigManager::getInstance()->getCopyOnDemand());
}

void test_parseDisableCopyOnDemand(void)
{
	CommandArgHelper cmds;
	cmds << "--disable-copy-on-demand";
	cmds.activate();
	cppcut_assert_equal(
	  ConfigManager::DISABLE,
	  ConfigManager::getInstance()->getCopyOnDemand());
}

} // namespace testConfigManager
