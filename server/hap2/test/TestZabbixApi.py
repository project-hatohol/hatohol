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
import haplib
import zabbixapi
import test_urllib2

class TestZabbixApi(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        zabbixapi.urllib2 = test_urllib2.urllib2()
        ms_dict = {"serverId": None, "url": None, "type":0, "nickName": "nick",
                   "userName": "test", "password": "test", "extendedInfo":"",
                   "pollingIntervalSec": 10, "retryIntervalSec":10}
        monitoring_server_info = haplib.MonitoringServerInfo(ms_dict)
        cls.api =  zabbixapi.ZabbixAPI(monitoring_server_info)

    def test_get_auth_token(self):
        result = self.api.get_auth_token("user", "password")
        exact = "test_auth_token"

        self.assertEquals(exact, result)

    def test_get_api_version(self):
        result = self.api.get_api_version()
        exact = "2.2.7"

    def test_get_items(self):
        result = self.api.get_items()
        exact = [{"itemId": u"1","hostId": u"1", "brief": u"test_name",
                  "unit": u"B", "itemGroupName": [u"test_name"],
                  "lastValueTime": "19700101000000.111111111", "lastValue": u"100"}]

        self.assertEquals(exact, result)

    def test_get_history(self):
        result = self.api.get_history("test_id", "19700101000000.111",
                                                 "19700101000000.222")
        exact = [{"value": u"1","time": "19700101000000.111111111"},
                 {"value": u"2","time": "19700101000001.222222222"}]

        self.assertEquals(exact, result)

    def test_get_item_value_type(self):
        result = self.api.get_item_value_type("test_id")
        exact = "3"

        self.assertEquals(exact, result)

    def test_get_host(self):
        result_hosts, result_host_group_membership = self.api.get_hosts()
        exact_hosts = [{"hostId": u"1", "hostName": u"test_host"}]
        exact_host_group_membership = [{"hostId": u"1", "groupIds":[u"1"]}]


        self.assertEquals(exact_hosts, result_hosts)
        self.assertEquals(exact_host_group_membership, result_host_group_membership)

    def test_host_groups(self):
        result = self.api.get_host_groups()
        exact = [{"groupId": "1", "groupName": "test_name"}]

        self.assertEquals(exact, result)

    def test_get_triggers(self):
        result = self.api.get_triggers()
        exact = [{"triggerId": u"1", "status": "OK", "severity": "ERROR",
                  "lastChangeTime": "19700101000000", "hostId": u"1", "hostName": u"test_host",
                  "brief": u"test_description",
                  "extendedInfo": u"test_expand_description"}]

        self.assertEquals(exact, result)

    def test_get_trigger_expand_description(self):
        result = self.api.get_trigger_expand_description()
        exact = {u"result": [{u"triggerid": u"1",
                 u"description": u"test_expand_description",
                 u"state": u"0", u"priority": 3}]}

        self.assertEquals(exact, result)

    def test_get_select_trigger(self):
        result = self.api.get_select_trigger("test_id")
        exact = {u"triggerid": u"1", u"priority": 3, u"state": u"0",
                 u"description": u"test_expand_description"}

        self.assertEquals(exact, result)

    def test_get_events(self):
        result = self.api.get_events("test_from")
        exact = [{"eventId": u"1", "time": u"19700101000000.111111111", "type": "GOOD",
                  "triggerId": u"1", "status": "OK", "severity": "ERROR",
                  "hostId": u"1", "hostName": u"test_host",
                  "brief": u"test_expand_description",
                  "extendedInfo": ""}]

        self.assertEquals(exact, result)

    def test_get_response_dict(self):
        result = self.api.get_response_dict("user.authenticate", "test_param",
                                            "test_header")
        exact = {u'result': u'test_auth_token'}

        self.assertEquals(exact, result)

    def test_get_item_groups(self):
        applications = [{"name": "test1"},{"name": "test2"}]
        result = zabbixapi.get_item_groups(applications)
        exact = ["test1", "test2"]

        self.assertEquals(exact, result)

    def test_check_response(self):
        res_dict = {"error": None}
        self.assertFalse(zabbixapi.check_response(res_dict))

        res_dict = {"result": None}
        self.assertTrue(zabbixapi.check_response(res_dict))
