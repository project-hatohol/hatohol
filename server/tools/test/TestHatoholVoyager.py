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

  def assert_url(self, arg_list, expect, expect_method=None,
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

  #
  # Test cases
  #
  def test_show_server(self):
    arg_list = ["show-server"]
    self.assert_url(arg_list, "http://localhost:33194/server")

  def test_show_server_with_id(self):
    arg_list = ["show-server", "1"]
    self.assert_url(arg_list, "http://localhost:33194/server?serverId=1")

  def test_show_trigger(self):
    arg_list = ["show-trigger"]
    self.assert_url(arg_list, "http://localhost:33194/trigger")

  def test_show_trigger_with_server_id(self):
    arg_list = ["show-trigger", "3"]
    self.assert_url(arg_list, "http://localhost:33194/trigger?serverId=3")

  def test_show_trigger_with_server_and_host_id(self):
    arg_list = ["show-trigger", "3", "2000"]
    self.assert_url(arg_list, "http://localhost:33194/trigger?serverId=3&hostId=2000")

  def test_show_trigger_with_server_host_and_trigger_id(self):
    arg_list = ["show-trigger", "3", "2000", "123456789"]
    self.assert_url(arg_list, "http://localhost:33194/trigger?serverId=3&hostId=2000&triggerId=123456789")

  def test_show_event(self):
    arg_list = ["show-event"]
    self.assert_url(arg_list, "http://localhost:33194/event")

  def test_show_item(self):
    arg_list = ["show-item"]
    self.assert_url(arg_list, "http://localhost:33194/item")

  def test_show_host(self):
    arg_list = ["show-host"]
    self.assert_url(arg_list, "http://localhost:33194/host")

  def test_show_host_with_server_id(self):
    arg_list = ["show-host", "3"]
    self.assert_url(arg_list, "http://localhost:33194/host?serverId=3")

  def test_show_host_with_server_and_host_id(self):
    arg_list = ["show-host", "3", "25600"]
    self.assert_url(arg_list, "http://localhost:33194/host?serverId=3&hostId=25600")
  def test_show_action(self):
    arg_list = ["show-action"]
    self.assert_url(arg_list, "http://localhost:33194/action")

  def test_add_action(self):
    ex_cmd = "ex-cmd -x --for ABC"
    arg_list = ["add-action", "--type", "command", "--command", ex_cmd]
    expect_query = {"type":hatohol.ACTION_COMMAND, "command":ex_cmd}
    self.assert_url(arg_list, "http://localhost:33194/action", None,
                    expect_query)

  def test_add_action_working_dir(self):
    ex_cmd = "ex-cmd -x --for ABC"
    workdir = "/tmp/foo/@@$$"
    arg_list = ["add-action", "--type", "command", "--command", ex_cmd,
                "--working-dir", workdir]
    expect_query = {"type":hatohol.ACTION_COMMAND, "command":ex_cmd,
                    "workingDirectory":workdir}
    self.assert_url(arg_list, "http://localhost:33194/action", None,
                    expect_query)

  def test_add_action_timeout(self):
    ex_cmd = "ex-cmd -x --for ABC"
    timeout = 305
    arg_list = ["add-action", "--type", "command", "--command", ex_cmd,
                "--timeout", str(timeout)]
    expect_query = {"type":hatohol.ACTION_COMMAND, "command":ex_cmd,
                    "timeout":str(timeout)}
    self.assert_url(arg_list, "http://localhost:33194/action", None,
                    expect_query)

  def test_add_action_server_id(self):
    ex_cmd = "ex-cmd -x --for ABC"
    server_id = 23
    arg_list = ["add-action", "--type", "command", "--command", ex_cmd,
                "--server-id", str(server_id)]
    expect_query = {"type":hatohol.ACTION_COMMAND, "command":ex_cmd,
                    "serverId":str(server_id)}
    self.assert_url(arg_list, "http://localhost:33194/action", None,
                    expect_query)

  def test_add_action_host_id(self):
    ex_cmd = "ex-cmd -x --for ABC"
    host_id = 0x890abcdef1234567
    arg_list = ["add-action", "--type", "command", "--command", ex_cmd,
                "--host-id", str(host_id)]
    expect_query = {"type":hatohol.ACTION_COMMAND, "command":ex_cmd,
                    "hostId":str(host_id)}
    self.assert_url(arg_list, "http://localhost:33194/action", None,
                    expect_query)

  def test_del_action(self):
    arg_list = ["del-action", "25"]
    self.assert_url(arg_list, "http://localhost:33194/action/25", "DELETE")

if __name__ == '__main__':
    unittest.main()
