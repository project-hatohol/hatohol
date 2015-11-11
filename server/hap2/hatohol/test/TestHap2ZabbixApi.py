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
import hap2_zabbix_api
import transporter
import haplib
import testutils

class PreviousHostsInfo(unittest.TestCase):
    def test_create(self):
        previous_hosts_info = hap2_zabbix_api.PreviousHostsInfo()
        self.assertEquals(previous_hosts_info.hosts, list())
        self.assertEquals(previous_hosts_info.host_groups, list())
        self.assertEquals(previous_hosts_info.host_group_membership, list())


class ZabbixAPIConductor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def null_func(*args, **kwargs):
            pass
        def return_empty_string(*args, **kwargs):
            return ""
        hap2_zabbix_api.ZabbixAPIConductor.get_component_code = null_func
        hap2_zabbix_api.ZabbixAPIConductor.get_sender = null_func
        cls.conductor = hap2_zabbix_api.ZabbixAPIConductor()

        cls.conductor._ZabbixAPIConductor__api =  APIForTest()
        cls.conductor.put_items = null_func
        cls.conductor.put_history = null_func
        cls.conductor.put_hosts = null_func
        cls.conductor.put_host_groups =  null_func
        cls.conductor.put_host_group_membership = null_func
        cls.conductor.put_triggers = null_func
        cls.conductor.put_events = null_func
        cls.conductor.get_last_info = null_func
        cls.conductor.get_cached_event_last_info = return_empty_string

    def test_reset(self):
        self.conductor.reset()
        api = testutils.get_priv_attr(self.conductor, "__api")
        self.assertIsNone(api)
        self.conductor._ZabbixAPIConductor__api =  APIForTest()

    def test_make_sure_token(self):
        pass

    def test_request(self):
        try:
            self.conductor.request("test_name", "test_params")
        except NotImplementedError as exc:
            self.assertEquals("", str(exc))

    def test_wait_acknowledge(self):
        try:
            self.conductor._wait_acknowledge("test_id")
        except NotImplementedError as exc:
            self.assertEquals("", str(exc))

    def test_wait_response(self):
        try:
            self.conductor._wait_response("self_id")
        except NotImplementedError as exc:
            self.assertEquals("", str(exc))

    def test_get_component_code(self):
        try:
            self.conductor.get_component_code()
        except NotImplementedError as exc:
            self.assertEquals("", str(exc))

    def test_get_sender(self):
        try:
            self.conductor.get_sender()
        except NotImplementedError as exc:
            self.assertEquals("", str(exc))

    def test_collect_and_put_items(self):
        testutils.assertNotRaises(self.conductor.collect_and_put_items,
                               "test_ids", "test_id")

    def test_collect_and_put_history(self):
        testutils.assertNotRaises(self.conductor.collect_and_put_history,
                               "test_id", "test_time", "test_time", "test_id")

    def test_update_hosts_and_host_group_membership(self):
        testutils.assertNotRaises(self.conductor.update_hosts_and_host_group_membership)

    def test_update_host_groups(self):
        testutils.assertNotRaises(self.conductor.update_host_groups)

    def test_update_triggers(self):
        testutils.assertNotRaises(self.conductor.update_triggers,
                               "test_ids", "test_id")

    def test_update_events_poll(self):
        testutils.assertNotRaises(self.conductor.update_events_poll)

    def test_update_events_fetch_asc(self):
        testutils.assertNotRaises(self.conductor.update_events_fetch,
                                  last_info=1, count=1, direction="ASC", fetch_id=1)

    def test_update_events_fetch_desc(self):
        testutils.assertNotRaises(self.conductor.update_events_fetch,
                                  last_info=1, count=1, direction="DESC", fetch_id=1)

    def test_update_hosts_and_host_group_membership(self):
        testutils.assertNotRaises(self.conductor.update_hosts_and_host_group_membership)


class Hap2ZabbixAPIMain(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        transporter_args = {"class": transporter.Transporter}
        sender = haplib.Sender(transporter_args)
        cls.main = hap2_zabbix_api.Hap2ZabbixAPIMain()
        cls.main.setup(transporter_args)

        cls.main._ZabbixAPIConductor__api =  APIForTest()
        def null_func(self, *args, **kwargs):
            pass
        cls.main.put_items = null_func
        cls.main.put_history = null_func
        cls.main.put_hosts = null_func
        cls.main.put_host_groups =  null_func
        cls.main.put_host_group_membership = null_func
        cls.main.put_triggers = null_func
        cls.main.put_events = null_func
        cls.main.get_last_info = null_func

    def test_hap_fetch_items(self):
        params = {"hostId": "1", "fetchId": 1}
        testutils.assertNotRaises(self.main.hap_fetch_items, params, 1)

    def test_hap_fetch_history(self):
        params = {"itemId": "1", "beginTime": None, "endTime": None, "fetchId": 1}
        testutils.assertNotRaises(self.main.hap_fetch_history, params, 1)

    def test_hap_fetch_triggers(self):
        params = {"hostId": ["1"], "fetchId": 1}
        testutils.assertNotRaises(self.main.hap_fetch_triggers, params, 1)

    def test_hap_fetch_events(self):
        params = {"lastInfo": 1, "count": 10, "direction": "ASC",
                       "fetchId": 1}
        testutils.assertNotRaises(self.main.hap_fetch_events, params, 1)


class Hap2ZabbixAPIPoller(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        transporter_args = {"class": transporter.Transporter}
        sender = haplib.Sender(transporter_args)
        cls.poller = hap2_zabbix_api.Hap2ZabbixAPIPoller(sender=sender,
                                                         process_id="test")

        cls.poller._ZabbixAPIConductor__api =  APIForTest()
        def null_func(self, *args, **kwargs):
            pass
        def return_empty_string(*args, **kwargs):
            return ""
        cls.poller.put_hosts = null_func
        cls.poller.put_host_groups =  null_func
        cls.poller.put_host_group_membership = null_func
        cls.poller.put_triggers = null_func
        cls.poller.put_events = null_func
        cls.poller.get_last_info = null_func
        cls.poller.get_cached_event_last_info = return_empty_string

    def test_poll(self):
        testutils.assertNotRaises(self.poller.poll)


class Hap2ZabbixAPI(unittest.TestCase, hap2_zabbix_api.Hap2ZabbixAPI):
    def test_create_main_plugin(self):
        transporter_args = {"class": transporter.Transporter}
        testutils.assertNotRaises(self.create_main_plugin, transporter_args=transporter_args)

    def test_create_poller(self):
        transporter_args = {"class": transporter.Transporter}
        sender = haplib.Sender(transporter_args)
        testutils.assertNotRaises(self.create_poller, sender=sender,
                               process_id="test")


class APIForTest:
    def get_items(self, host_id=None):
        test_items = [{"unit": "example unit",
                       "itemGroupName": "example name",
                       "lastValue": "example value",
                       "lastValueTime": "20150410175500",
                       "brief": "example brief",
                       "hostId": "1",
                       "itemId": "1"}]

        return test_items

    def get_history(self, item_id, begin_time, end_time):
        test_history = [{"time": "20150323113000", "value": "exampleValue"},
                        {"time": "20150323113012", "value": "exampleValue2"}]

        return test_history

    def get_hosts(self):
        test_hosts = [{"hostId": "1", "hostName": "test_host_name"}]
        test_host_group_membership = [{"hostId": "1", "groupIds": ["1", "2"]}]

        return (test_hosts, test_host_group_membership)

    def get_host_groups(self):
        test_host_groups = [{"groupId": "1", "groupName": "test_group"}]

        return test_host_groups

    def get_triggers(self, request_since=None, host_id=None):
        test_triggers = [{"extendedInfo": "sample extended info",
                          "brief": "example brief",
                          "hostName": "exampleName",
                          "hostId": "1",
                          "lastChangeTime": "20150323175800",
                          "severity": "INFO",
                          "status": "OK",
                          "triggerId": "1"}]

        return test_triggers

    def get_events(self, event_id_from, event_id_till=None):
        test_events = [{"extendedInfo": "sampel extended info",
                        "brief": "example brief",
                        "eventId": "1",
                        "time": "20150323151300",
                        "type": "GOOD",
                        "triggerId": 2,
                        "status": "OK",
                        "severity": "INFO",
                        "hostId": 3,
                        "hostName": "exampleName"}]

        return test_events

    def get_end_id(self, is_first):
        test_event_id = 1

        return test_event_id
