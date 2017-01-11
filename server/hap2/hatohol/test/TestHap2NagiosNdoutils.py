#!/usr/bin/env python
# coding: UTF-8
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
import testutils
from hatohol import hap
from hatohol import haplib
from hap2_nagios_ndoutils import Common
import hap2_nagios_ndoutils
import transporter

class CommonForTest(Common):
    def __init__(self, options={}):
        Common.__init__(self)
        self.__options = options
        self.stores = {}
        print options

    def get_ms_info(self):
        if self.__options.get("db_invalid_param"):
            return haplib.MonitoringServerInfo({
                "serverId": "hoge",
                "url": "",
                "type": "",
                "nickName": "",
                "userName": "non-existing-user",
                "password": "",
                "pollingIntervalSec": 30,
                "retryIntervalSec": 10,
                "extendedInfo": "",
            })
        else:
            # On TravisCI, we may return appropriate paramters here
            return None

    def put_hosts(self, hosts):
        self.stores["hosts"] = hosts

    def put_host_groups(self, groups):
        self.stores["host_groups"] = groups

    def put_host_group_membership(self, membership):
        self.stores["host_group_membership"] = membership

    def put_triggers(self, triggers, put_empty_contents, update_type,
                     last_info=None, fetch_id=None):
        self.stores["triggers"] = triggers
        self.stores["update_type"] = update_type
        self.stores["last_info"] = last_info
        self.stores["fetch_id"] = fetch_id

    def get_cached_event_last_info(self):
        return None

    def put_events(self, events, put_empty_contents,
                   fetch_id=None, last_info_generator=None):
        self.stores["events"] = events
        self.stores["fetch_id"] = fetch_id
        self.stores["last_info_generator"] = last_info_generator

    def divide_and_put_data(self, put_func, contents, *args, **kwargs):
        put_func(contents, *args, **kwargs)

    def set_event_last_info(self, last_info):
        pass


class TestCommon(unittest.TestCase):
    def test_constructor(self):
        testutils.assertNotRaises(Common)

    def test_ensure_connection(self):
        comm = CommonForTest()
        testutils.assertNotRaises(comm.ensure_connection)

    def test_ensure_connection_with_failure_of_opening_db(self):
        options = {"db_invalid_param": True}
        comm = CommonForTest(options)
        self.assertRaises(hap.Signal, comm.ensure_connection)

    def test_close_connection_without_connection(self):
        comm = Common()
        testutils.assertNotRaises(comm.close_connection)

    def test_close_connection(self):
        self.test_ensure_connection()
        self.test_close_connection_without_connection()

    def test_parse_url_sv_port_db(self):
        self.__assert_parse_url(
            "123.45.67.89:1122/mydb", ("123.45.67.89", 1122, "mydb"))

    def test_parse_url_sv_port(self):
        self.__assert_parse_url(
            "123.45.67.89:1122",
            ("123.45.67.89", 1122, Common.DEFAULT_DATABASE))

    def test_parse_url_sv(self):
        self.__assert_parse_url(
            "123.45.67.89",
            ("123.45.67.89", Common.DEFAULT_PORT, Common.DEFAULT_DATABASE))

    def test_parse_url_sv_db(self):
        self.__assert_parse_url(
            "123.45.67.89/mydb",
            ("123.45.67.89", Common.DEFAULT_PORT, "mydb"))

    def test_parse_url_db(self):
        self.__assert_parse_url(
            "/mydb", (Common.DEFAULT_SERVER, Common.DEFAULT_PORT, "mydb"))

    def test_collect_hosts_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        # TODO: insert test materials and check it
        comm.collect_hosts_and_put()
        self.assertEquals(type(comm.stores["hosts"]), type([]))

    def test_collect_host_groups_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        # TODO: insert test materials and check it
        comm.collect_host_groups_and_put()
        self.assertEquals(type(comm.stores["host_groups"]), type([]))

    def test_collect_host_group_membership_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        # TODO: insert test materials and check it
        comm.collect_host_group_membership_and_put()
        self.assertEquals(type(comm.stores["host_group_membership"]), type([]))

    def test_collect_triggers_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        # TODO: insert test materials and check it
        fetch_id = "12345"
        comm.collect_triggers_and_put(fetch_id)
        self.assertEquals(type(comm.stores["triggers"]), type([]))
        self.assertEquals(comm.stores["update_type"], "ALL")
        self.assertEquals(comm.stores["last_info"], None)
        self.assertEquals(comm.stores["fetch_id"], fetch_id)

    def test_collect_events_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        fetch_id = "6789"
        last_info = None
        count = None
        direction = "ASC"
        # TODO: insert test materials and check it
        comm.collect_events_and_put(fetch_id, last_info, count, direction)
        self.assertEquals(type(comm.stores["events"]), type([]))
        self.assertEquals(comm.stores["fetch_id"], fetch_id)

    def test_validate_object_ids(self):
        self.__assert_validate_object_ids(["123", "555555", "23434324234"])

    def test_validate_object_ids_with_empty_data(self):
        self.__assert_validate_object_ids([])

    def test_validate_object_ids_with_invalid_data(self):
        self.__assert_validate_object_ids(["123", "555555", "I'm Mike"], False)

    # NOTE: The following tests for single id not (ids), different from
    #       above ones
    def test_validate_object_id(self):
        self.__assert_validate_object_id("123")

    def test_validate_object_with_empty_id(self):
        self.__assert_validate_object_id("", False)

    def test_validate_object_with_string(self):
        self.__assert_validate_object_id("String", False)

    def test_validate_object_with_hex(self):
        self.__assert_validate_object_id("a5a5a5", False)

    def test_extract_validated_event_last_info(self):
        self.__assert_extract_validated_event_last_info("12345", 12345)

    def test_extract_validated_event_last_info_with_negative(self):
        self.__assert_extract_validated_event_last_info("-12", None)

    def test_extract_validated_event_last_info_with_empty(self):
        self.__assert_extract_validated_event_last_info("", None)

    def test_extract_validated_event_last_info_with_string(self):
        self.__assert_extract_validated_event_last_info("An apple", None)

    def test_parse_status_and_severity_OK(self):
        status = 0
        self.__assert_parse_status_and_severity(status, ("OK", "INFO"))

    def test_parse_status_and_severity_WARNING(self):
        status = 1
        self.__assert_parse_status_and_severity(status, ("NG", "WARNING"))

    def test_parse_status_and_severity_CRITICAL(self):
        status = 2
        self.__assert_parse_status_and_severity(status, ("NG", "CRITICAL"))

    def test_parse_status_and_severity_with_invalid_status(self):
        status = 15
        self.__assert_parse_status_and_severity(status, ("UNKNOWN", "UNKNOWN"))

    def test_parse_status_and_severity_with_invalid_stringstatus(self):
        status = "ABC"
        self.__assert_parse_status_and_severity(status, ("UNKNOWN", "UNKNOWN"))

    #
    # Utility methods
    #
    def __assert_parse_url(self, url, expect):
        comm = Common()
        target_func = testutils.get_priv_attr(comm, "__parse_url")
        self.assertEquals(target_func(url), expect)

    def __assert_validate_object_ids(self, host_ids, expect=True):
        comm = Common()
        target_func = testutils.get_priv_attr(comm, "__validate_object_ids")
        self.assertEquals(target_func(host_ids), expect)

    def __assert_validate_object_id(self, host_id, expect=True):
        comm = Common()
        target_func = testutils.get_priv_attr(comm, "__validate_object_id")
        self.assertEquals(target_func(host_id), expect)

    def __assert_extract_validated_event_last_info(self, last_info, expect):
        comm = Common()
        target_func = \
            testutils.get_priv_attr(comm, "__extract_validated_event_last_info")
        self.assertEquals(target_func(last_info), expect)

    def __assert_parse_status_and_severity(self, status, expect):
        comm = Common()
        target_func = \
            testutils.get_priv_attr(comm, "__parse_status_and_severity")
        self.assertEquals(target_func(status), expect)


class TraceableTestCommon:
    def __init__(self):
        self.stores = {"trace":[]}

    def ensure_connection(self):
        self.stores["trace"].append("ensure_connection")

    def collect_hosts_and_put(self):
        self.stores["trace"].append("collect_host_and_put")

    def collect_host_groups_and_put(self):
        self.stores["trace"].append("collect_host_groups_and_put")

    def collect_host_group_membership_and_put(self):
        self.stores["trace"].append("collect_host_group_membership_and_put")

    def collect_triggers_and_put(self, fetch_id=None, host_ids=None):
        self.stores["trace"].append("collect_triggers_and_put")
        self.stores["fetch_id"] = fetch_id
        self.stores["host_ids"] = host_ids

    def collect_events_and_put(self, fetch_id=None, last_info=None, count=None,
                               direction=None):
        self.stores["trace"].append("collect_events_and_put")
        self.stores["fetch_id"] = fetch_id
        self.stores["last_info"] = last_info
        self.stores["count"] = count
        self.stores["direction"] = direction


class PollerForTest(TraceableTestCommon,
                    hap2_nagios_ndoutils.Hap2NagiosNDOUtilsPoller):
    def __init__(self):
        kwargs = {"sender": "", "process_id": "PollerForTest"}
        hap2_nagios_ndoutils.Hap2NagiosNDOUtilsPoller.__init__(self, **kwargs)
        TraceableTestCommon.__init__(self)


class Hap2NagiosNDOUtilsPoller(unittest.TestCase):
    def test_constructor(self):
        kwargs = {"sender": "", "process_id": "my test"}
        poller = hap2_nagios_ndoutils.Hap2NagiosNDOUtilsPoller(**kwargs)

    def test_poll(self):
        poller = PollerForTest()
        poller.poll()
        expected_traces = [
            "ensure_connection",
            "collect_host_and_put",
            "collect_host_groups_and_put",
            "collect_host_group_membership_and_put",
            "collect_triggers_and_put",
            "collect_events_and_put",
        ]
        self.assertEquals(poller.stores["trace"], expected_traces)


class MainPluginForTest(TraceableTestCommon,
                        hap2_nagios_ndoutils.Hap2NagiosNDOUtilsMain):
    def __init__(self):
        hap2_nagios_ndoutils.Hap2NagiosNDOUtilsMain.__init__(self)
        self.setup({"class": transporter.Transporter})
        TraceableTestCommon.__init__(self)


class Hap2NagiosNDOUtilsMain(unittest.TestCase):
    def test_constructor(self):
        main = hap2_nagios_ndoutils.Hap2NagiosNDOUtilsMain()

    def __assert_hap_fetch_triggers(self, hostIds):
        main = MainPluginForTest()
        params = {"fetchId": "252525"}
        if hostIds is not None:
            params["hostIds"] = hostIds
        request_id = "1234"
        main.hap_fetch_triggers(params, request_id)
        self.assertEquals(main.stores["trace"],
                          ["ensure_connection", "collect_triggers_and_put"])
        self.assertEquals(main.stores["fetch_id"], params["fetchId"])
        self.assertEquals(main.stores["host_ids"], params.get("hostIds"))

    def test_hap_fetch_triggers(self):
        self.__assert_hap_fetch_triggers(["12", "345", "678"])

    def test_hap_fetch_triggers_with_none_hostIds(self):
        self.__assert_hap_fetch_triggers(None)

    def test_hap_fetch_events(self):
        main = MainPluginForTest()
        params = {"fetchId": "252525", "lastInfo": "abcedef",
                  "count": 100, "direction": "ASC"}
        request_id = "1234"
        main.hap_fetch_events(params, request_id)
        self.assertEquals(main.stores["trace"],
                          ["ensure_connection", "collect_events_and_put"])
        self.assertEquals(main.stores["fetch_id"], params["fetchId"])
        self.assertEquals(main.stores["last_info"], params["lastInfo"])
        self.assertEquals(main.stores["count"], params["count"])
        self.assertEquals(main.stores["direction"], params["direction"])

class Hap2NagiosNDOUtils(unittest.TestCase):
    def test_create_main_plugin(self):
        hap = hap2_nagios_ndoutils.Hap2NagiosNDOUtils()
        main_plugin = hap.create_main_plugin()
        expect_class = hap2_nagios_ndoutils.Hap2NagiosNDOUtilsMain
        self.assertTrue(isinstance(main_plugin, expect_class))

    def test_create_poller(self):
        hap = hap2_nagios_ndoutils.Hap2NagiosNDOUtils()
        kwargs = {"sender": None, "process_id": ""}
        poller = hap.create_poller(**kwargs)
        expect_class = hap2_nagios_ndoutils.Hap2NagiosNDOUtilsPoller
        self.assertTrue(isinstance(poller, expect_class))
