#!/usr/bin/env python
"""
  Copyright (C) 2013 Project Hatohol

  This file is part of Hatohol.

  Hatohol is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  Hatohol is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
"""
import unittest
import urllib
import urllib2

import hatohol
from hatohol import voyager

class TestHatoholEscapeException:
  pass

class TestHatoholVoyager(unittest.TestCase):

  def _assert_url(self, arg_list, expect, expect_method=None,
                  expect_query=None):
    ctx = {"url":None}
    def url_hook(hook_url, encoded_query):
      if isinstance(hook_url, urllib2.Request):
        ctx["url"] = hook_url.get_full_url()
        ctx["method"] = hook_url.get_method()
      else:
        ctx["url"] = hook_url

      if encoded_query is not None:
        ctx["encoded_query"] = encoded_query
      else:
        ctx["encoded_query"] = None
      raise TestHatoholEscapeException()

    voyager.set_url_open_hook(url_hook)
    try:
      voyager.main(arg_list)
    except TestHatoholEscapeException:
      pass
    actual = ctx["url"]
    self.assertEquals(actual, expect)
    if expect_method is not None:
      self.assertEquals(ctx["method"], expect_method)
    if expect_query is not None:
      self.assertEquals(ctx["encoded_query"], urllib.urlencode(expect_query))

  def _assert_add_action_one_opt(self, option, opt_value,
                                 query_key, query_value):
    ex_cmd = "ex-cmd -x --for ABC"
    arg_list = ["add-action", "--type", "command", "--command", ex_cmd,
                option, opt_value]
    expect_query = {"type":hatohol.ACTION_COMMAND, "command":ex_cmd,
                    query_key:query_value}
    self._assert_url(arg_list, "http://localhost:33194/action", None,
                     expect_query)

  def _assert_severity(self, comparator, comparator_value):
    # TODO: add error, emergency
    severities = (("info", hatohol.TRIGGER_SEVERITY_INFO),
                  ("warn", hatohol.TRIGGER_SEVERITY_WARNING),
                  ("critical", hatohol.TRIGGER_SEVERITY_CRITICAL))
    for (severity_label, severity_value) in severities:
      ex_cmd = "ex-cmd -x --for ABC"
      arg_list = ["add-action", "--type", "command", "--command", ex_cmd,
                  "--severity", comparator, severity_label]
      print arg_list
      expect_query = {"type":hatohol.ACTION_COMMAND, "command":ex_cmd,
                      "triggerSeverityCompType":str(comparator_value),
                      "triggerSeverity":str(severity_value)}
      self._assert_url(arg_list, "http://localhost:33194/action", None,
                       expect_query)

  #
  # Test cases
  #
  def test_show_server(self):
    arg_list = ["show-server"]
    self._assert_url(arg_list, "http://localhost:33194/server")

  def test_show_server_with_id(self):
    arg_list = ["show-server", "1"]
    self._assert_url(arg_list, "http://localhost:33194/server?serverId=1")

  def test_show_trigger(self):
    arg_list = ["show-trigger"]
    self._assert_url(arg_list, "http://localhost:33194/trigger")

  def test_show_trigger_with_server_id(self):
    arg_list = ["show-trigger", "3"]
    self._assert_url(arg_list, "http://localhost:33194/trigger?serverId=3")

  def test_show_trigger_with_server_and_host_id(self):
    arg_list = ["show-trigger", "3", "2000"]
    self._assert_url(arg_list, "http://localhost:33194/trigger?serverId=3&hostId=2000")

  def test_show_trigger_with_server_host_and_trigger_id(self):
    arg_list = ["show-trigger", "3", "2000", "123456789"]
    self._assert_url(arg_list, "http://localhost:33194/trigger?serverId=3&hostId=2000&triggerId=123456789")

  def test_show_event(self):
    arg_list = ["show-event"]
    self._assert_url(arg_list, "http://localhost:33194/event")

  def test_show_item(self):
    arg_list = ["show-item"]
    self._assert_url(arg_list, "http://localhost:33194/item")

  def test_show_host(self):
    arg_list = ["show-host"]
    self._assert_url(arg_list, "http://localhost:33194/host")

  def test_show_host_with_server_id(self):
    arg_list = ["show-host", "3"]
    self._assert_url(arg_list, "http://localhost:33194/host?serverId=3")

  def test_show_host_with_server_and_host_id(self):
    arg_list = ["show-host", "3", "25600"]
    self._assert_url(arg_list, "http://localhost:33194/host?serverId=3&hostId=25600")
  def test_show_action(self):
    arg_list = ["show-action"]
    self._assert_url(arg_list, "http://localhost:33194/action")

  def test_add_action(self):
    ex_cmd = "ex-cmd -x --for ABC"
    arg_list = ["add-action", "--type", "command", "--command", ex_cmd]
    expect_query = {"type":hatohol.ACTION_COMMAND, "command":ex_cmd}
    self._assert_url(arg_list, "http://localhost:33194/action", None,
                    expect_query)

  def test_add_action_resident(self):
    ex_cmd = "foo.so -x --for ABC"
    arg_list = ["add-action", "--type", "resident", "--command", ex_cmd]
    expect_query = {"type":hatohol.ACTION_RESIDENT, "command":ex_cmd}
    self._assert_url(arg_list, "http://localhost:33194/action", None,
                    expect_query)

  def test_add_action_working_dir(self):
    workdir = "/tmp/foo/@@$$"
    self._assert_add_action_one_opt("--working-dir", workdir,
                                    "workingDirectory", workdir)

  def test_add_action_timeout(self):
    timeout = 305
    self._assert_add_action_one_opt("--timeout", str(timeout),
                                    "timeout", str(timeout))

  def test_add_action_server_id(self):
    server_id = 23
    self._assert_add_action_one_opt("--server-id", str(server_id),
                                    "serverId", str(server_id))

  def test_add_action_host_id(self):
    host_id = 0x890abcdef1234567
    self._assert_add_action_one_opt("--host-id", str(host_id),
                                    "hostId", str(host_id))

  def test_add_action_host_group_id(self):
    host_group_id = 0xa0b1c2d3e4f50617
    self._assert_add_action_one_opt("--host-group-id", str(host_group_id),
                                    "hostGroupId", str(host_group_id))

  def test_add_action_trigger_id(self):
    trigger_id = 0xfedcba9876543210
    self._assert_add_action_one_opt("--trigger-id", str(trigger_id),
                                    "triggerId", str(trigger_id))

  def test_add_action_status_ok(self):
    self._assert_add_action_one_opt("--status", "ok", "triggerStatus",
                                    str(hatohol.TRIGGER_STATUS_OK))

  def test_add_action_status_problem(self):
    self._assert_add_action_one_opt("--status", "problem", "triggerStatus",
                                    str(hatohol.TRIGGER_STATUS_PROBLEM))

  def test_add_action_status_serverity(self):
    severity_cmp = (("eq", hatohol.CMP_EQ), ("ge", hatohol.CMP_EQ_GT))
    for (comparator, value) in severity_cmp:
      self._assert_severity(comparator, value)

  def test_del_action(self):
    arg_list = ["del-action", "25"]
    self._assert_url(arg_list, "http://localhost:33194/action/25", "DELETE")

if __name__ == '__main__':
    unittest.main()
