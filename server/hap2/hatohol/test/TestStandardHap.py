#!/usr/bin/env python
"""
  Copyright (C) 2015-2016 Project Hatohol

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
from hatohol import transporter
import multiprocessing
import json
import signal
import time
import standardhap

def flush_sigchild(wait_time=3):
    """
    This method aims to wait for SIGCHLD, which is sent
    due to the end of processes that were created in the previous tests.
    If the signal is received when a test is running, the test would fail.
    """
    def _handler(signum, frame):
        print "SIGCHLD flusher: Got a signal."

    print "SIGCHLD flusher: Start..."
    signal.signal(signal.SIGCHLD, _handler)
    t_end = time.time() + wait_time
    while True:
       t_sleep = t_end - time.time()
       if t_sleep < 0:
            break
       time.sleep(t_sleep)
    print "SIGCHLD flusher: Finished."


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


transporter.Manager.register(EzTransporter)


class TestStandardHap(unittest.TestCase):
    def setUp(self):
        flush_sigchild()

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
        sys.argv = [sys.argv[0], "--transporter", "EzTransporter"]
        hap = self.StandardHapTestee()
        hap()
        hap.enable_handling_sigchld(False)
        expect_ms = haplib.MonitoringServerInfo(json.loads(EzTransporter.TEST_MONITORING_SERVER_RESULT))
        actual_ms = hap.get_received_ms_info()
        self.assertEqual(str(actual_ms), str(expect_ms))
