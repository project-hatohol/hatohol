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
import testutils
from hap2_ceilometer import Common
import hap2_ceilometer
from datetime import datetime
from hatohol import hap
from hatohol import haplib
import re
import transporter

class CommonForTest(Common):

    NOVA_EP = "http://hoge/nova"
    CEILOMETER_EP = "http://hoge/ceilometer"

    SERVERS = [{
        "id": "12345",
        "name": "onamae",
    }]

    ALARMS = [{
        "alarm_id": "112233",
        "state_timestamp": "2015-06-29T10:05:11.135000",
        "description": "DESCRIPTION",
        "state": "ok",
        "threshold_rule": {
            "meter_name": "cpu_util",
            "query": [{
                "field": "name",
                "value": "889900",
                "op": "eq",
            }]
        },
    }]


    EXPECT_ITEM_HOST_ID1_CNAME = {
        "itemId": "host_id1.CNAME",
        "hostId": "host_id1",
        "brief": "CNAME",
        "lastValueTime": "19940525123456.000000",
        "lastValue": "CVOL",
        "itemGroupName": "",
        "unit": "UNIT",
    }

    def __init__(self, options={}):
        Common.__init__(self)
        self.__options = options
        self.store = {}

        # replace a lower layer method
        self._Common__request = self.__request

    def get_ms_info(self):
        if self.__options.get("none_monitoring_server_info"):
            return None
        return haplib.MonitoringServerInfo({
            "serverId": 51,
            "url": "http://example.com",
            "type": "Ceilometer",
            "nickName": "Jack",
            "userName": "fooo",
            "password": "gooo",
            "pollingIntervalSec": 30,
            "retryIntervalSec": 10,
            "extendedInfo": '{"tenantName": "yah"}',
        })

    def put_hosts(self, hosts):
        self.store["hosts"] = hosts

    def put_triggers(self, triggers, put_empty_contents, update_type,
                     last_info=None, fetch_id=None):
        self.store["triggers"] = triggers
        self.store["update_type"] = update_type
        self.store["last_info"] = last_info
        self.store["fetch_id"] = fetch_id

    def put_events(self, events, put_empty_contents,
                   fetch_id=None, last_info_generator=None):
        self.store["events"] = events
        self.store["fetch_id"] = fetch_id
        self.store["last_info_generator"] = last_info_generator

    def __request(self, url, headers={}, use_token=True, data=None):
        url_handler_map = {
            "http://example.com/tokens": self.__request_token,
            self.NOVA_EP + "/servers": self.__request_servers,
            "http://hoge/ceilometer/v2/.*/history":
                self.__request_alarm_history,
            "http://hoge/ceilometer/v2/alarms": self.__request_alarms,
            "http://hoge/ceilometer/v2/resource": self.__request_resources,
            "http://HREF/href1": self.__request_href1,
            "http://hoge/ceilometer/v2/meters/CNAME":
                self.__request_meters_CNAME,
        }

        handler = None
        for key, func in url_handler_map.items():
            if re.search(key, url):
                handler = func
                break
        else:
            raise Exception("Not found handler for %s" % url)
        return handler(url)

    def __request_token(self, url):
        return {
            "access": {
                "token": {
                    "id": "xxxxxxxxxxxxxxxx",
                    "expires": "2015-06-29T16:54:23Z",
                },
                "serviceCatalog": [{
                    "name": "nova",
                    "endpoints": [{"publicURL": self.NOVA_EP}],
                }, {
                    "name": "ceilometer",
                    "endpoints": [{"publicURL": self.CEILOMETER_EP}],
                }]
            }
        }

    def __request_servers(self, url):
        return {
            "servers": self.SERVERS,
        }

    def __request_alarms(self, url):
        return self.ALARMS

    def __request_alarm_history(self, url):
        return [{
            "type": "state transition",
            "detail": '{"state": "alarm"}',
            "timestamp": "2013-03-22T04:56:12.111222",
            "event_id": "event_id_2",
        }, {
            "type": "state transition",
            "detail": '{"state": "ok"}',
            "timestamp": "2013-03-22T04:56:00.334455",
            "event_id": "event_id_1",
        }, {
            "type": "creation",
            "detail": '{"state": "ok"}',
            "timestamp": "2013-03-22T04:50:00.334455",
            "event_id": "event_id_0",
        }]

    def __request_resources(self, url):
        return {
            "links": [{
                "rel": "cpu_util",
                "href": "http://HREF/href1",
            }],
        }

    def __request_href1(self, url):
        num_items = self.__options.get("href1_num_items", 1)
        items = [{
            "timestamp": "1994-05-25T12:34:56",
            "counter_name": "CNAME",
            "counter_volume": "CVOL",
            "counter_unit": "UNIT",
        }, {
            "timestamp": "1997-01-22T22:33:44",
            "counter_name": "Flower",
            "counter_volume": 13.20,
            "counter_unit": "%",
        }, {
            "timestamp": "1992-08-20T11:22:33",
            "counter_name": "Lake",
            "counter_volume": 123,
            "counter_unit": "degree",
        }]
        return items[0:num_items]

    def __request_meters_CNAME(self, url):
        return [{
            "timestamp": "2012-12-19T01:23:46",
            "counter_volume": 12,
        }, {
            "timestamp": "2012-12-19T01:23:45",
            "counter_volume": 10,
        }]

    def get_cached_event_last_info(self):
        return "abcdef"

    def put_items(self, items, put_empty_contents, fetch_id):
        self.store["items"] = items
        self.store["fetch_id"] = fetch_id

    def put_history(self, samples, put_history, item_id, fetch_id):
        self.store["samples"] = samples
        self.store["item_id"] = item_id
        self.store["fetch_id"] = fetch_id

    def set_host_cache(self, host_cache):
        """
        Set the given dictionary to Common.__host_cache
        @host_cache A dictionary in which
                    the key is host_id and the value is host name.
        """
        self._Common__host_cache = host_cache

    def divide_and_put_data(self, put_func, contents, *args, **kwargs):
        put_func(contents, *args, **kwargs)


class TestCommon(unittest.TestCase):
    def test_constructor(self):
        testutils.assertNotRaises(Common)

    def test_ensure_connection_without_monitoring_server_info(self):
        options = {"none_monitoring_server_info": True}
        comm = CommonForTest(options)
        self.assertRaises(hap.Signal, comm.ensure_connection)

    def test_ensure_connection(self):
        options = {}
        comm = CommonForTest(options)
        comm.ensure_connection()
        nova_ep = testutils.get_priv_attr(comm, "__nova_ep", "Common")
        self.assertEqual(nova_ep, comm.NOVA_EP)
        ceilometer_ep = \
            testutils.get_priv_attr(comm, "__ceilometer_ep", "Common")
        self.assertEqual(ceilometer_ep, comm.CEILOMETER_EP)

    # skip tests for __set_nova_ep and __set_ceilometer_ep, because theya are
    # private and used in ensure_connection().

    def test_collect_hosts_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        comm.collect_hosts_and_put()
        hosts = comm.store["hosts"]
        self.assertEqual(hosts, [{"hostId": "12345", "hostName": "onamae"}])

    def test_collect_triggers_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()

        fetch_id = "000111"
        # TODO: Test with non-None host_ids
        host_ids = None
        comm.collect_triggers_and_put(fetch_id=fetch_id, host_ids=host_ids)
        self.assertEqual(comm.store["triggers"],
            [{
                "triggerId": "112233",
                "status": "OK",
                "severity": "ERROR",
                "lastChangeTime": "20150629100511.135000",
                "hostId": "889900",
                "hostName": "N/A",
                "brief": "cpu_util: DESCRIPTION",
                "extendedInfo": "",
             }])
        self.assertEquals(comm.store["update_type"], "ALL")
        self.assertEquals(comm.store["last_info"], None)
        self.assertEquals(comm.store["fetch_id"], fetch_id)

    def test_collect_events_and_put(self):
        caught = []
        def catcher(alarm_id, last_alarm_time, fetch_id):
            caught.extend([alarm_id, last_alarm_time, fetch_id])

        comm = CommonForTest()
        comm.ensure_connection()
        fetch_id = "000111"
        # TODO: Test with non-None host_ids, last_info, count
        last_info = None
        host_ids = None
        count = None
        # TODO: Test with "DESC"
        direction = "ASC"
        comm._Common__collect_events_and_put = catcher
        comm._Common__decode_last_alarm_timestamp_map = \
            lambda x: {"alarm_id1": "20150629110435.250000"}
        comm._Common__alarm_cache = {"alarm_id1": {"host id", "brief"}}
        comm.collect_events_and_put(fetch_id=fetch_id,
                                    last_info=last_info,
                                    count=count, direction=direction)
        self.assertEqual(caught,
                         ["alarm_id1", "20150629110435.250000", "000111"])


    def test_collect_items_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        # TODO: Test with non-None host_ids
        fetch_id = "000111"
        host_ids = ["host_id1"]
        comm.collect_items_and_put(fetch_id, host_ids)
        self.assertEquals(comm.store["items"], [
            {
                "itemId": "host_id1.CNAME",
                "hostId": "host_id1",
                "brief": "CNAME",
                "lastValueTime": "19940525123456.000000",
                "lastValue": "CVOL",
                "itemGroupName": "",
                "unit": "UNIT",
            }
        ])
        self.assertEquals(comm.store["fetch_id"], fetch_id)

    def test_collect_history_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        fetch_id = "0055"
        host_id = "host_id1"
        item_id = "host_id1.CNAME"
        # TODO: Set the proper time range
        begin_time = ""
        end_time = ""
        comm.collect_history_and_put(fetch_id=fetch_id, host_id=host_id,
                                     item_id=item_id, begin_time=begin_time,
                                     end_time=end_time)
        self.assertEquals(comm.store["fetch_id"], fetch_id)
        self.assertEquals(comm.store["item_id"], item_id)
        self.assertEquals(comm.store["samples"], [
            {
                "time": "20121219012345.000000",
                "value": "10",
            }, {
                "time": "20121219012346.000000",
                "value": "12",
            }
        ])

    def test__collect_items_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        host_id = "host_id2"
        target_func = \
            testutils.get_priv_attr(comm, "__collect_items_and_put", "Common")
        expect_item = comm.EXPECT_ITEM_HOST_ID1_CNAME
        expect_item["itemId"] = "host_id2.CNAME"
        expect_item["hostId"] = host_id
        self.assertEquals(target_func(host_id), [expect_item])

    def __assert_get_resource(self, num_items, expect):
        comm = CommonForTest({"href1_num_items": num_items})
        get_resource = \
            testutils.get_priv_attr(comm, "__get_resource", "Common")
        rel = None
        href = "http://HREF/href1"
        self.assertEquals(get_resource(rel, href), expect)

    def test_get_resource(self):
        self.__assert_get_resource(1, {
            "timestamp": "1994-05-25T12:34:56",
            "counter_name": "CNAME",
            "counter_unit": "UNIT",
            "counter_volume": "CVOL"
        })

    def test_get_resource_with_no_returned_item(self):
        self.__assert_get_resource(0, None)

    def test_get_resource_with_two_returned_item(self):
        self.__assert_get_resource(2, {
            "timestamp": "1994-05-25T12:34:56",
            "counter_name": "CNAME",
            "counter_unit": "UNIT",
            "counter_volume": "CVOL"
        })

    def __assert_decode_last_alarm_timestamp_map(self, last_info, source):
        comm = CommonForTest()
        dec_func = testutils.get_priv_attr(
                        comm, "__decode_last_alarm_timestamp_map", "Common")
        self.assertEquals(dec_func(last_info), source)

    def test_encode_decode_last_alarm_timestamp_map(self):
        comm = CommonForTest()
        enc_func = testutils.get_priv_attr(
                        comm, "__encode_last_alarm_timestamp_map", "Common")
        source = {"ABCDEFG": 1, "2345": "foo"}
        encoded = enc_func(source)
        self.__assert_decode_last_alarm_timestamp_map(encoded, source)

    def test_decode_last_alarm_timestamp_map_with_none(self):
        self.__assert_decode_last_alarm_timestamp_map(None, {})

    def test_decode_last_alarm_timestamp_map_with_invalid_input(self):
        self.assertRaises(Exception,
            self.__assert_decode_last_alarm_timestamp_map, ("NON ENCODED", {}))

    def test__collect_events_and_put(self):
        comm = CommonForTest()
        comm.ensure_connection()
        target_func = testutils.get_priv_attr(comm, "__collect_events_and_put",
                                              "Common")
        # TODO: test the path when alarm_cache is hit
        #       In that case, hostId, hostName, and brief should not be "N/A"
        alarm_id = "alarm1"
        last_alarm_time = "20150423112233.123000"
        fetch_id = "a123"
        target_func(alarm_id, last_alarm_time, fetch_id)
        self.assertEquals(comm.store["fetch_id"], fetch_id)
        self.assertEquals(comm.store["last_info_generator"],
            testutils.get_priv_attr(comm, "__last_info_generator", "Common"))
        self.assertEquals(comm.store["events"], [
            {
                "eventId": "event_id_0",
                "time": "20130322045000.334455",
                "type": "GOOD",
                "status": "OK",
                "triggerId": alarm_id,
                "hostId": "N/A",
                "hostName": "N/A",
                "severity": "ERROR",
                "brief": "N/A",
                "extendedInfo": "",
            }, {
                "eventId": "event_id_1",
                "time": "20130322045600.334455",
                "status": "OK",
                "type": "GOOD",
                "triggerId": alarm_id,
                "hostId": "N/A",
                "hostName": "N/A",
                "severity": "ERROR",
                "brief": "N/A",
                "extendedInfo": "",
            }, {
                "eventId": "event_id_2",
                "time": "20130322045612.111222",
                "type": "BAD",
                "status": "NG",
                "triggerId": alarm_id,
                "hostId": "N/A",
                "hostName": "N/A",
                "severity": "ERROR",
                "brief": "N/A",
                "extendedInfo": "",
            }])

    def test_last_info_generator(self):
        comm = CommonForTest()
        target_func = testutils.get_priv_attr(comm, "__last_info_generator",
                                              "Common")
        # TODO: test the path when __alarm_last_time_map.get(alarm_id) is hit
        events = [{
            "triggerId": "alarm1",
            "time": "20121212121212.121212",
        }, {
            "triggerId": "alarm2",
            "time": "20121212121213.131313",
        }]
        last_info = target_func(events)
        expect =  {
            "alarm1": "20121212121212.121212",
            "alarm2": "20121212121213.131313",
        }
        self.__assert_decode_last_alarm_timestamp_map(last_info, expect)

    def test_remove_missing_alarm_before_get_all_alarms(self):
        comm = CommonForTest()
        target_func = testutils.get_priv_attr(comm, "__remove_missing_alarm",
                                              "Common")
        alarm_time_map = {"alarm1": "20120222101011.123456",
                          "alarm2": "20120322121311.123456",
        }
        target_func(alarm_time_map)
        self.assertEquals(len(alarm_time_map), 2)

    def test_remove_missing_alarm(self):
        comm = CommonForTest()
        comm.ensure_connection()
        comm.collect_triggers_and_put()
        target_func = testutils.get_priv_attr(comm, "__remove_missing_alarm",
                                              "Common")
        alarm_time_map = {"alarm1": "20120222101011.123456",
                          "112233": "20120322121311.123456",
        }
        target_func(alarm_time_map)
        self.assertEquals(alarm_time_map, {"112233": "20120322121311.123456"})


class Common__get_history_query_option(unittest.TestCase):
    def __assert_get_history_query_option(self, last_alarm_time, expect):
        comm = CommonForTest()
        target_func = testutils.get_priv_attr(
                            comm, "__get_history_query_option", "Common")
        self.assertEquals(target_func(last_alarm_time), expect)

    def test_get_history_query_option_with_none_input(self):
        self.__assert_get_history_query_option(None, "")

    def test_get_history_query_option(self):
        self.__assert_get_history_query_option(
            "20130703121015.000456",
            "?q.field=timestamp&q.op=gt&q.value=2013-07-03T12%3A10%3A15.000456")


class Common_hapi_time_to_url_enc_openstack_time(unittest.TestCase):
    def test_hapi_time_to_url_enc_openstack_time(self):
        hapi_time = "20150624081005"
        actual = Common.hapi_time_to_url_enc_openstack_time(hapi_time)
        expect = "2015-06-24T08%3A10%3A05.000000"
        self.assertEqual(actual, expect)

    def test_hapi_time_to_url_enc_openstack_time_with_us(self):
        hapi_time = "20150624081005.123456"
        actual = Common.hapi_time_to_url_enc_openstack_time(hapi_time)
        expect = "2015-06-24T08%3A10%3A05.123456"
        self.assertEqual(actual, expect)

    def test_hapi_time_to_url_enc_openstack_time_with_ns(self):
        hapi_time = "20150624081005.123456789"
        actual = Common.hapi_time_to_url_enc_openstack_time(hapi_time)
        expect = "2015-06-24T08%3A10%3A05.123456"
        self.assertEqual(actual, expect)

    def test_hapi_time_to_url_enc_openstack_time_with_ms(self):
        hapi_time = "20150624081005.123"
        actual = Common.hapi_time_to_url_enc_openstack_time(hapi_time)
        expect = "2015-06-24T08%3A10%3A05.123000"
        self.assertEqual(actual, expect)

    def test_hapi_time_to_url_enc_openstack_time_with_dot_only(self):
        hapi_time = "20150624081005."
        actual = Common.hapi_time_to_url_enc_openstack_time(hapi_time)
        expect = "2015-06-24T08%3A10%3A05.000000"
        self.assertEqual(actual, expect)


# TODO: Implemnet Test for __request


class Common__parse_alarm_host(unittest.TestCase):
    def __assert_parse_alarm_host(self, threshold_rule,  expect,
                                  host_cache=None):
        comm = CommonForTest()
        if host_cache is not None:
            comm.set_host_cache(host_cache)
        target_func = testutils.get_priv_attr(
                            comm, "__parse_alarm_host", "Common")
        self.assertEquals(target_func(threshold_rule), expect)

    def test_parse_alarm_host_without_query(self):
        self.__assert_parse_alarm_host({}, ("N/A", "N/A"))

    def test_parse_alarm_host(self):
        threshold_rule = {
            "query": [{
                "field": "host",
                "value": "host12345",
                "op": "eq",
            }]
        }
        comm = CommonForTest()
        host_cache = {"host12345": "onamae755"}
        self.__assert_parse_alarm_host(
            threshold_rule, ("host12345", "onamae755"), host_cache)

    def test_parse_alarm_host_with_all_invalid_query(self):
        threshold_rule = {
            "query": [{
            }]
        }
        comm = CommonForTest()
        self.__assert_parse_alarm_host(threshold_rule, ("N/A", "N/A"))


class Common__parse_alarm_host_each(unittest.TestCase):
    def __assert(self, query, expect):
        comm = CommonForTest()
        target_func = testutils.get_priv_attr(
                            comm, "__parse_alarm_host_each", "Common")
        self.assertEquals(target_func(query), expect)

    def test_no_field(self):
        self.__assert({"value":"hoge", "op":"eq"}, None)

    def test_no_value(self):
        self.__assert({"field":"hoge", "op":"eq"}, None)

    def test_no_op(self):
        self.__assert({"field":"hoge", "value": "foo"}, None)

    def test_op_is_not_eq(self):
        self.__assert({"field":"hoge", "value": "foo", "op":"gt"}, None)

    def test_normal(self):
        self.__assert({"field":"hoge", "value": "foo", "op":"eq"}, "foo")


class Common__fixup_event_last_info(unittest.TestCase):
    def __assert(self, last_info, expect, cached_last_info=""):
        comm = CommonForTest()
        target = testutils.get_priv_attr(comm, "__fixup_event_last_info",
                                         "Common")
        comm.get_cached_event_last_info = lambda: cached_last_info
        self.assertEquals(target(last_info), expect)

    def test_none(self):
        self.__assert(None, None)

    def test_none_with_cached(self):
        self.__assert(None, "cached", "cached")

    def test_empty_string(self):
        self.__assert("", None)

    def test_non_empty_string(self):
        self.__assert("abc", "abc")


class Common_alarm_to_hapi_status(unittest.TestCase):
    def test_alarm_to_hapi_status_ok(self):
        alarm_type = "state transition"
        detail = '{"state": "ok"}'
        evt_type = Common.alarm_to_hapi_status(alarm_type, detail)
        self.assertEqual(evt_type, "OK")

    def test_alarm_to_hapi_status_ng(self):
        alarm_type = "state transition"
        detail = '{"state": "alarm"}'
        evt_type = Common.alarm_to_hapi_status(alarm_type, detail)
        self.assertEqual(evt_type, "NG")

    def test_alarm_to_hapi_status_unknown(self):
        alarm_type = "state transition"
        detail = '{"state": "insufficient data"}'
        evt_type = Common.alarm_to_hapi_status(alarm_type, detail)
        self.assertEqual(evt_type, "UNKNOWN")

    def test_alarm_to_hapi_status_creation(self):
        alarm_type = "creation"
        detail = '{"state": "ok"}'
        evt_type = Common.alarm_to_hapi_status(alarm_type, detail)
        self.assertEqual(evt_type, "OK")

    def test_alarm_to_hapi_status_with_invalid_type(self):
        alarm_type = "unknown type"
        detail = '{"state": "ok"}'
        self.assertRaises(
            Exception,
            Common.alarm_to_hapi_status, (alarm_type, detail))

    def test_alarm_to_hapi_status_with_invalid_detail(self):
        alarm_type = "state transition"
        detail = '{"state": "Cho Cho II kanji"}'
        self.assertRaises(
            Exception,
            Common.alarm_to_hapi_status, (alarm_type, detail))

class Common_status_to_hapi_event_type(unittest.TestCase):
    def __assert(self, status, expect):
        self.assertEquals(Common.status_to_hapi_event_type(status), expect)

    def test_OK(self):
        self.__assert("OK", "GOOD")

    def test_NG(self):
        self.__assert("NG", "BAD")

    def test_UNKNOWN(self):
        self.__assert("UNKNOWN", "UNKNOWN")

    def test_others(self):
        self.assertRaises(KeyError, Common.status_to_hapi_event_type,
                          ("others"))


class Common_parse_time(unittest.TestCase):
    def test_parse_time_with_micro(self):
        actual = Common.parse_time("2014-09-05T06:25:29.185000")
        expect = datetime(2014, 9, 5, 6, 25, 29, 185000)
        self.assertEqual(actual, expect)

    def test_parse_time_without_micro(self):
        actual = Common.parse_time("2014-09-05T06:25:29")
        expect = datetime(2014, 9, 5, 6, 25, 29)
        self.assertEqual(actual, expect)

    def test_parse_time_without_invalid_string(self):
        self.assertRaises(Exception, Common.parse_time, "20140905062529")


# TODO: Extract common part with tests for hap2_nagios_udoutils
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

    def collect_items_and_put(self, fetch_id, host_ids):
        self.stores["trace"].append("collect_items_and_put")
        self.stores["fetch_id"] = fetch_id
        self.stores["host_ids"] = host_ids

    def collect_history_and_put(self, fetch_id, host_id, item_id,
                                begin_time, end_time):
        self.stores["trace"].append("collect_history_and_put")
        self.stores["fetch_id"] = fetch_id
        self.stores["host_id"] = host_id
        self.stores["item_id"] = item_id
        self.stores["begin_time"] = begin_time
        self.stores["end_time"] = end_time


class PollerForTest(TraceableTestCommon, hap2_ceilometer.Hap2CeilometerPoller):
    def __init__(self):
        kwargs = {"sender": "", "process_id": "PollerForTest"}
        TraceableTestCommon.__init__(self)
        hap2_ceilometer.Hap2CeilometerPoller.__init__(self, **kwargs)


class Hap2CeilometerPoller(unittest.TestCase):
    def test_constructor(self):
        kwargs = {"sender": "", "process_id": "my test"}
        poller = hap2_ceilometer.Hap2CeilometerPoller(**kwargs)

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
                        hap2_ceilometer.Hap2CeilometerMain):
    def __init__(self):
        TraceableTestCommon.__init__(self)
        hap2_ceilometer.Hap2CeilometerMain.__init__(self)
        self.setup({"class": transporter.Transporter})


# TODO: code-sharing between tests for NagiosNdoUtils and ZabbixApi
class Hap2CeilometerMain(unittest.TestCase):
    def test_constructor(self):
        main = hap2_ceilometer.Hap2CeilometerMain()

    def __assert_hap_fetch_triggers(self, host_ids):
        main = MainPluginForTest()
        params = {"fetchId": "252525"}
        if host_ids is not None:
            params["hostIds"] = host_ids
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

    def __assert_hap_fetch_items(self, host_ids):
        main = MainPluginForTest()
        params = {"fetchId": "252525"}
        if host_ids is not None:
            params["hostIds"] = host_ids
        request_id = "1234"
        main.hap_fetch_items(params, request_id)
        self.assertEquals(main.stores["trace"],
                          ["ensure_connection", "collect_items_and_put"])
        self.assertEquals(main.stores["fetch_id"], params["fetchId"])
        self.assertEquals(main.stores["host_ids"], params.get("hostIds"))

    def test_hap_fetch_items(self):
        self.__assert_hap_fetch_items(["12", "345", "678"])

    def test_hap_fetch_items_with_none_hostIds(self):
        self.__assert_hap_fetch_items(None)

    def test_hap_fetch_history(self):
        main = MainPluginForTest()
        params = {"fetchId": "252525", "hostId": "12", "itemId": "334455",
                  "beginTime": "20141212012345.875000",
                  "endTime": "20150102032345.000000"}
        request_id = "1234"
        main.hap_fetch_history(params, request_id)
        self.assertEquals(main.stores["trace"],
                          ["ensure_connection", "collect_history_and_put"])
        self.assertEquals(main.stores["fetch_id"], params["fetchId"])
        self.assertEquals(main.stores["host_id"], params["hostId"])
        self.assertEquals(main.stores["item_id"], params["itemId"])
        self.assertEquals(main.stores["begin_time"], params["beginTime"])
        self.assertEquals(main.stores["end_time"], params["endTime"])

class Hap2Ceilometer(unittest.TestCase):
    def test_create_main_plugin(self):
        hap = hap2_ceilometer.Hap2Ceilometer()
        kwargs = {"transporter_args": {"class": transporter.Transporter}}
        main_plugin = hap.create_main_plugin(**kwargs)
        expect_class = hap2_ceilometer.Hap2CeilometerMain
        self.assertTrue(isinstance(main_plugin, expect_class))

    def test_create_poller(self):
        hap = hap2_ceilometer.Hap2Ceilometer()
        kwargs = {"sender": None, "process_id": ""}
        poller = hap.create_poller(**kwargs)
        expect_class = hap2_ceilometer.Hap2CeilometerPoller
        self.assertTrue(isinstance(poller, expect_class))
