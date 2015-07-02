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
import common as testutils
import hap2_fluentd
import transporter
import haplib
import os
import datetime

class Hap2FluentdMainTestee(hap2_fluentd.Hap2FluentdMain):
    def __init__(self):
        kwargs = {"transporter_args": {"class": transporter.Transporter}}
        hap2_fluentd.Hap2FluentdMain.__init__(self, **kwargs)
        self.stores = {}

    def get_launch_args(self):
        return self.get_priv_attr("__launch_args")

    def set_null_fluentd_manager_main(self):
        self._Hap2FluentdMain__fluentd_manager_main = lambda : None

    def get_priv_attr(self, name):
        return testutils.returnPrivObj(self, name, "Hap2FluentdMain")

    def put_events(self, events, fetch_id=None, last_info_generator=None):
        self.stores["events"] = events
        self.stores["fetch_id"] = fetch_id
        self.stores["last_info_generator"] = last_info_generator

class Hap2FluentdMain(unittest.TestCase):
    def test_constructor(self):
        kwargs = {"transporter_args": {"class": transporter.Transporter}}
        main = hap2_fluentd.Hap2FluentdMain(**kwargs)

    def test_set_arguments(self):
        main = Hap2FluentdMainTestee()
        arg = type('', (), {})()
        arg.fluentd_launch = "ABC -123 435"
        main.set_arguments(arg)
        self.assertEquals(main.get_launch_args(), ["ABC", "-123", "435"])

    def test_set_ms_info(self):
        main = Hap2FluentdMainTestee()
        main.set_null_fluentd_manager_main()
        main.set_ms_info(haplib.MonitoringServerInfo({
            "serverId": 51,
            "url": "http://example.com",
            "type": "Fluentd",
            "nickName": "Jack",
            "userName": "fooo",
            "password": "gooo",
            "pollingIntervalSec": 30,
            "retryIntervalSec": 10,
            "extendedInfo": "",
        }))

    def test__fluentd_manager_main(self):
        memory = {}
        def alternate():
            memory["called"] = True
        main = Hap2FluentdMainTestee()
        main._Hap2FluentdMain__fluentd_manager_main_in_try_block = alternate
        main._Hap2FluentdMain__fluentd_manager_main()
        self.assertTrue(memory["called"])

    def test__fluentd_manager_main_in_try_block(self):
        footmarks = []
        def alt_put_event(timestamp, tag, raw_msg):
            footmarks.extend((timestamp, tag, raw_msg))
            raise Exception

        main = Hap2FluentdMainTestee()
        main._Hap2FluentdMain__put_event = alt_put_event
        arg = type('', (), {})()
        curr_dir = os.path.dirname(os.path.abspath(__file__))
        arg.fluentd_launch = curr_dir + "/hap2-fluentd-tester.sh"
        main.set_arguments(arg)
        target_func = main.get_priv_attr("__fluentd_manager_main_in_try_block")
        self.assertRaises(Exception, target_func)
        ts, tag, msg = footmarks
        self.assertEquals(ts, datetime.datetime(2015, 7, 22, 1, 23, 45))
        self.assertEquals(tag, "hatohol.test")
        self.assertEquals(msg, "test message\n")

    def test__put_event(self):
        main = Hap2FluentdMainTestee()
        target_func = main.get_priv_attr("__put_event")
        timestamp = datetime.datetime(2015, 7, 22, 1, 23, 45)
        tag = "fooo"
        raw_msg = '{"message": "Hello!", "host": "chaos"}'
        target_func(timestamp, tag, raw_msg)

        # event ID is calculated base on the time (microsecond)
        # So we check it by comparing the current time
        actual_event_id = long(main.stores["events"][0]["eventId"])
        curr_event_id = \
            long(datetime.datetime.utcnow().strftime("%Y%m%d%H%M%S%f"))
        ALLOWED_ERROR_USEC = 1000 * 1000 # micro second
        self.assertLess(curr_event_id - actual_event_id, ALLOWED_ERROR_USEC)

        del main.stores["events"][0]["eventId"]
        self.assertEquals(main.stores["events"], [{
            "time": "20150722012345.000000",
            "type": "UNKNOWN",
            "status": "UNKNOWN",
            "hostId": "chaos",
            "hostName": "chaos",
            "severity": "UNKNOWN",
            "brief": "Hello!",
            "extendedInfo": "",
        }])
        self.assertEquals(main.stores["fetch_id"], None)
        arg = 1 # The value can be arbitrary since it's not used actually.
        self.assertEquals(main.stores["last_info_generator"](arg), None)

    # TODO: make tests for __generate_event_id, __parse_header.
    #       Note that they are indirectly used in the above test.


class Hap2FluentdMain__parse_line(unittest.TestCase):
    def __assert(self, line, expects):
        main = Hap2FluentdMainTestee()
        target_func = main.get_priv_attr("__parse_line")
        self.assertEquals(target_func(line), expects)

    def test_normal(self):
        expect_ts = datetime.datetime(2015, 7, 2, 9, 20, 20)
        self.__assert(
            "2015-07-02 18:20:20 +0900 [warn]: process died",
            (expect_ts, "[warn]", "process died"))

    def test_unexpected_date_format(self):
        expect_ts = datetime.datetime(2015, 7, 2, 9, 20, 20)
        self.__assert(
            "20150702182020 +0900 [warn]: process died", (None, None, None))

    def test_without_colon(self):
        expect_ts = datetime.datetime(2015, 7, 2, 9, 20, 20)
        self.__assert("just message", (None, None, None))


class Hap2FluentdMain__get_paramter(unittest.TestCase):
    def __assert(self, msg, key, default_value, candidates, expect):
        main = Hap2FluentdMainTestee()
        target_func = main.get_priv_attr("__get_parameter")
        actual = target_func(msg, key, default_value, candidates)
        self.assertEquals(actual, expect)

    def test_empty(self):
        self.__assert({}, "key", "default", {}, "default")

    def test_normal(self):
        self.__assert({"key":"value"}, "key", "default", ("value",), "value")

    def test_not_in_candidates(self):
        self.__assert({"key":"value"}, "key", "default", ("Mushi",), "default")

class Hap2Fluentd(unittest.TestCase):
    def test_create_main_plugin(self):
        hap = hap2_fluentd.Hap2Fluentd()
        arg = type('', (), {})()
        arg.fluentd_launch = ""
        hap.on_parsed_argument(arg)
        kwargs = {"transporter_args": {"class": transporter.Transporter}}
        main_plugin = hap.create_main_plugin(**kwargs)
        expect_class = hap2_fluentd.Hap2FluentdMain
        self.assertTrue(isinstance(main_plugin, expect_class))
