#!/usr/bin/env python
"""
  Copyright (C) 2015 Project Hatohol

  This file is part of Hatohol.

  Hatohol is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License, version 3
  as published by the Free Software Foundation.

  Hatohol is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Hatohol. If not, see
  <http://www.gnu.org/licenses/>.
"""
import unittest
import sys
from standardhap import StandardHap
import haplib
import transporter
import multiprocessing
import json

class EzTransporter(transporter.Transporter):
    __queue = multiprocessing.Queue()
    TEST_MONITORING_SERVER_RESULT = '{ \
          "extendedInfo": "exampleExtraInfo", \
          "serverId": 1, \
          "url": "http://example.com:80", \
          "type": 0, \
          "nickName": "exampleName", \
          "userName": "Admin", \
          "password": "examplePass",\
          "pollingIntervalSec": 30, \
          "retryIntervalSec": 10 \
        }'
    TEST_EXCHANGE_PROFILE_RESULT = '["exchangeProfile"]'

    def __init__(self):
        transporter.Transporter.__init__(self)
        self.run_receive_counter = 2

    def call(self, _msg):
        cmd_map = {"exchangeProfile": self.__exchange_profile, "getMonitoringServerInfo": self.__get_monitoring_server_info}
        msg = json.loads(_msg)
        handler = cmd_map.get(msg["method"])
        assert handler is not None
        handler(msg["id"], msg["params"])

    def reply(self, msg):
        raise Error

    def run_receive_loop(self):
        while self.run_receive_loop > 0:
            msg = self.__queue.get()
            (self.get_receiver())(None, msg)
            self.run_receive_counter -= 1

    def __exchange_profile(self, call_id, params):
        reply = '{"id": %s, "result": %s, "jsonrpc": "2.0"}' % \
                (str(call_id), self.TEST_EXCHANGE_PROFILE_RESULT)
        self.__queue.put(reply)

    def __get_monitoring_server_info(self, call_id, params):
        reply = '{"id": %s, "result": %s, "jsonrpc": "2.0"}' % \
                (str(call_id), self.TEST_MONITORING_SERVER_RESULT)
        self.__queue.put(reply)


class TestStandardHap(unittest.TestCase):
    class StandardHapTestee(StandardHap):
        def __init__(self):
            StandardHap.__init__(self)
            self.__received_ms_info = None
            self.__main_plugin = None

        def create_main_plugin(self, *args, **kwargs):
            self.__main_plugin = haplib.BaseMainPlugin(*args, **kwargs)
            return self.__main_plugin

        def on_got_monitoring_server_info(self, ms_info):
            self.__received_ms_info = ms_info
            self.__main_plugin.request_exit()

        def get_received_ms_info(self):
            return self.__received_ms_info

    def test_normal_run(self):
        hap = self.StandardHapTestee()
        sys.argv = [sys.argv[0],
                    "--transporter", "EzTransporter",
                    "--transporter-module", "test.TestStandardHap"]
        hap()
        hap.enable_handling_sigchld(False)
        exact_ms = haplib.MonitoringServerInfo(json.loads(EzTransporter.TEST_MONITORING_SERVER_RESULT))
        result_ms = hap.get_received_ms_info()

        self.assertTrue(result_ms, exact_ms)
