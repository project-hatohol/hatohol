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

import urllib2
import json
import datetime
import cPickle
import base64
from logging import getLogger
from hatohol import hap
from hatohol import haplib
from hatohol import standardhap
from hatohol import hapcommon

logger = getLogger("hatohol.hap2_ceilometer:%s" % hapcommon.get_top_file_name())

class Common:

    STATUS_MAP = {"ok": "OK", "insufficient data": "UNKNOWN", "alarm": "NG"}
    STATUS_EVENT_MAP = {"OK": "GOOD", "NG": "BAD", "UNKNOWN": "UNKNOWN"}
    INITIAL_LAST_INFO = ""

    def __init__(self):
        self.close_connection()
        self.__target_items = frozenset((
            "cpu",
            "cpu_util",
            "disk.read.requests",
            "disk.read.requests.rate",
            "disk.read.bytes",
            "disk.read.bytes.rate",
            "disk.write.requests",
            "disk.write.requests.rate",
            "disk.write.bytes",
            "disk.write.bytes.rate",
        ))

    def close_connection(self):
        self.__token = None
        self.__nova_ep = None
        self.__ceilometer_ep = None
        self.__host_cache = {} # key: host_id, value: host_name
        self.__alarm_cache = {} # key: alarm_id, value: {host_id, brief}

        # key: alarm_id, value: last_alarm time (HAPI format)
        self.__alarm_last_time_map = {}
        self.__alarm_cache_has_all = False

    def ensure_connection(self):
        if self.__token is not None:
            RETOKEN_MARGIN = datetime.timedelta(minutes=5)
            now  = datetime.datetime.utcnow()
            if self.__expires - now > RETOKEN_MARGIN:
                return

        self.__ms_info = self.get_ms_info()
        ms_info = self.__ms_info
        if ms_info is None:
            logger.error("Not found: MonitoringServerInfo.")
            raise hap.Signal()

        auth_url = ms_info.url + "/tokens"
        ext_info_type = haplib.MonitoringServerInfo.EXTENDED_INFO_JSON
        ext_info = ms_info.get_extended_info(ext_info_type)
        data = {
            "auth": {
                "tenantName": ext_info["tenantName"],
                "passwordCredentials": {
                    "username": ms_info.user_name,
                    "password": ms_info.password,
                }
            }
        }
        headers = {"Content-Type": "application/json"}
        response = self.__request(auth_url, headers, use_token=False,
                                  data=json.dumps(data))

        self.__token = response["access"]["token"]["id"]
        expires = response["access"]["token"]["expires"]
        self.__expires = datetime.datetime.strptime(expires,
                                                    "%Y-%m-%dT%H:%M:%SZ")
        logger.info("Got token, expires: %s" % self.__expires)

        # Extract endpoints
        target_eps = {"nova": self.__set_nova_ep,
                      "ceilometer": self.__set_ceilometer_ep}
        for catalog in response["access"]["serviceCatalog"]:
            if len(target_eps) == 0:
                break
            name = catalog["name"]
            ep_setter = target_eps.get(name)
            if ep_setter is None:
                continue
            ep_setter(catalog["endpoints"][0]["publicURL"])
            del target_eps[name]

        if len(target_eps) > 0:
            logger.error("Not found Endpoints: Nova: %s, Ceiloemeter: %s" % \
                          (self.__nova_ep, self.__ceilometer_ep))
            raise hap.Signal()

        logger.info("EP: Nova: %s", self.__nova_ep)
        logger.info("EP: Ceiloemeter: %s", self.__ceilometer_ep)

    def __set_nova_ep(self, ep):
        self.__nova_ep = ep

    def __set_ceilometer_ep(self, ep):
        self.__ceilometer_ep = ep

    def __collect_hosts(self):
        url = self.__nova_ep + "/servers/detail?all_tenants=1"
        response = self.__request(url)

        hosts = []
        for server in response["servers"]:
            host_id = server["id"]
            host_name = server["name"]
            hosts.append({"hostId": host_id, "hostName": host_name})
            self.__host_cache[host_id] = host_name
        return hosts

    def collect_hosts_and_put(self):
        self.divide_and_put_data(self.put_hosts, self.__collect_hosts())

    def collect_host_groups_and_put(self):
        pass

    def collect_host_group_membership_and_put(self):
        pass

    def collect_triggers_and_put(self, fetch_id=None, host_ids=None):
        url = self.__ceilometer_ep + "/v2/alarms";
        response = self.__request(url)

        # Now we get all the alarms. So the list shoud be cleared here
        self.__alarm_cache = {}
        self.__alarm_cache_has_all = False
        triggers = []
        for alarm in response:
            alarm_id = alarm["alarm_id"]
            threshold_rule = alarm["threshold_rule"]
            host_id, host_name = self.__parse_alarm_host(threshold_rule)
            if host_ids is not None:
                if host_id not in host_ids:
                    continue

            meter_name = threshold_rule["meter_name"]
            ts = datetime.datetime.strptime(alarm["state_timestamp"],
                                            "%Y-%m-%dT%H:%M:%S.%f")
            timestamp_str = ts.strftime("%Y%m%d%H%M%S.") + str(ts.microsecond)

            brief = "%s: %s" % (meter_name, alarm["description"])
            trigger = {
                "triggerId": alarm["alarm_id"],
                "status": self.STATUS_MAP[alarm["state"]],
                "severity": "ERROR",
                "lastChangeTime": timestamp_str,
                "hostId": host_id,
                "hostName": host_name,
                "brief": brief,
                "extendedInfo": "",
            }
            triggers.append(trigger)
            self.__alarm_cache[alarm_id] = {
                "host_id": host_id, "brief": brief}

        self.__alarm_cache_has_all = (host_ids is None)
        update_type = "ALL"

        put_empty_contents = True
        if fetch_id is None:
            put_empty_contents = False

        self.divide_and_put_data(self.put_triggers, triggers,
                                 put_empty_contents, update_type=update_type,
                                 fetch_id=fetch_id)

    def collect_events_and_put(self, fetch_id=None, last_info=None,
                               count=None, direction="ASC"):
        last_info = self.__fixup_event_last_info(last_info)
        last_alarm_timestamp_map = \
            self.__decode_last_alarm_timestamp_map(last_info)
        for alarm_id in self.__alarm_cache.keys():
            last_alarm_time = last_alarm_timestamp_map.get(alarm_id)
            self.__collect_events_and_put(alarm_id, last_alarm_time, fetch_id)

    def collect_items_and_put(self, fetch_id, host_ids):
        items = []
        if host_ids is None:
            host_ids = [obj["hostId"] for obj in self.__collect_hosts()]
        for host_id in host_ids:
            items.extend(self.__collect_items_and_put(host_id))

        self.divide_and_put_data(self.put_items, items, True, fetch_id)

    def collect_history_and_put(self, fetch_id, host_id, item_id,
                                begin_time, end_time):
        meter_name = item_id.split(".", 1)[1]
        base_url = "%s/v2/meters/%s" % (self.__ceilometer_ep, meter_name)
        query1 = "?q.field=resource_id&q.field=timestamp&q.field=timestamp"
        query2 = "&q.op=eq&q.op=gt&q.op=lt"
        t_beg = self.hapi_time_to_url_enc_openstack_time(begin_time)
        t_end = self.hapi_time_to_url_enc_openstack_time(end_time)
        query3 = "&q.value=%s&q.value=%s&q.value=%s" % (host_id, t_beg, t_end)

        url = base_url + query1 + query2 + query3
        response = self.__request(url)

        samples = []
        for history in response:
            timestamp = self.parse_time(history["timestamp"])
            hapi_time = hapcommon.conv_to_hapi_time(timestamp)
            samples.append({
                "time": hapi_time,
                "value": str(history["counter_volume"]),
            })
        sorted_samples = sorted(samples, key=lambda s: s["time"])
        self.divide_and_put_data(self.put_history, sorted_samples, True,
                                 item_id, fetch_id)

    def __collect_items_and_put(self, host_id):
        url = "%s/v2/resources/%s" % (self.__ceilometer_ep, host_id)
        response = self.__request(url)

        items = []
        for links in response["links"]:
            rel = links.get("rel")
            if rel not in self.__target_items:
                continue
            href = links.get("href")
            if href is None:
                continue
            rc = self.__get_resource(rel, href)
            if rc is None:
                continue

            timestamp = self.parse_time(rc["timestamp"])
            hapi_time = hapcommon.conv_to_hapi_time(timestamp)
            counter_name = rc["counter_name"]
            items.append({
                # Item ID must be unique so we generate it with the host ID
                # and the counter name.
                "itemId": host_id + "." + counter_name,
                "hostId": host_id,
                "brief": counter_name,
                "lastValueTime": hapi_time,
                "lastValue": str(rc["counter_volume"]),
                "itemGroupName": "",
                "unit": rc["counter_unit"],
            })
        return items

    def __get_resource(self, rel, href):
        url = href + "&limit=1";
        response = self.__request(url)
        len_resources = len(response)
        if len_resources == 0:
            logger.warning("Number of resources: %s: 0." % rel)
            return None
        if len_resources >= 2:
            msg = "Number of resources: %s: %d. We use the first one" \
                  % (rel, len_resources)
            logger.warning(msg)
        return response[0]

    def __decode_last_alarm_timestamp_map(self, last_info):
        if last_info is None:
            return {}
        try:
            pickled = base64.b64decode(last_info)
            last_alarm_timestamp_map = cPickle.loads(pickled)
        except Exception as e:
            logger.error("Failed to decode: %s."  % e)
            raise
        return last_alarm_timestamp_map

    def __encode_last_alarm_timestamp_map(self, last_alarm_timestamp_map):
        pickled = cPickle.dumps(last_alarm_timestamp_map)
        b64enc = base64.b64encode(pickled)
        return b64enc

    def __collect_events_and_put(self, alarm_id, last_alarm_time, fetch_id):
        query_option = self.__get_history_query_option(last_alarm_time)
        url = self.__ceilometer_ep + \
              "/v2/alarms/%s/history%s" % (alarm_id, query_option)
        response = self.__request(url)

        # host_id, host_name and brief
        alarm_cache = self.__alarm_cache.get(alarm_id)
        if alarm_cache is not None:
            host_id = alarm_cache["host_id"]
            brief = alarm_cache["brief"]
        else:
            host_id = "N/A"
            brief = "N/A"
        host_name = self.__host_cache.get(host_id, "N/A")

        # make the events to put
        events = []
        for history in response:
            hapi_status = self.alarm_to_hapi_status(
                        history["type"], history.get("detail"))
            hapi_event_type = self.status_to_hapi_event_type(hapi_status)
            timestamp = self.parse_time(history["timestamp"])

            events.append({
                "eventId": history["event_id"],
                "time": hapcommon.conv_to_hapi_time(timestamp),
                "type": hapi_event_type,
                "triggerId": alarm_id,
                "status": hapi_status,
                "severity": "ERROR",
                "hostId": host_id,
                "hostName": host_name,
                "brief": brief,
                "extendedInfo": ""
            })
        sorted_events = sorted(events, key=lambda evt: evt["time"])

        put_empty_contents = True
        if fetch_id is None:
            put_empty_contents = False

        self.divide_and_put_data(self.put_events, sorted_events,
                                 put_empty_contents, fetch_id=fetch_id,
                                 last_info_generator=self.__last_info_generator)

    def __last_info_generator(self, events):
        for evt in events:
            alarm_id = evt["triggerId"]
            alarm_time = evt["time"]

            doUpdate = True
            latest_time = self.__alarm_last_time_map.get(alarm_id)
            if latest_time is not None:
                # TODO by 15.09: FIX: This is too easy and imprecise
                # Memo: The precision is about three places of decimals.
                doUpdate = float(alarm_time) > float(latest_time)
            if doUpdate:
                self.__alarm_last_time_map[alarm_id] = alarm_time

        self.__remove_missing_alarm(self.__alarm_last_time_map)
        last_info = \
            self.__encode_last_alarm_timestamp_map(self.__alarm_last_time_map)
        assert len(last_info) <= haplib.MAX_LAST_INFO_SIZE
        return last_info

    def __remove_missing_alarm(self, alarm_time_map):
        if not self.__alarm_cache_has_all:
            # We cloud get all the alarms from OpenStack, however, current implementation
            # fails to do so to avoid long time to get them.
            return

        for alarm_id in alarm_time_map.keys():
            if alarm_id not in self.__alarm_cache:
                del alarm_time_map[alarm_id]

    def __get_history_query_option(self, last_alarm_time):
        if last_alarm_time is None:
            return ""
        time_value = self.hapi_time_to_url_enc_openstack_time(last_alarm_time)
        return "?q.field=timestamp&q.op=gt&q.value=%s" % time_value

    @staticmethod
    def hapi_time_to_url_enc_openstack_time(hapi_time):
        year  = hapi_time[0:4]
        month = hapi_time[4:6]
        day   = hapi_time[6:8]
        hour  = hapi_time[8:10]
        min   = hapi_time[10:12]
        sec   = hapi_time[12:14]
        if hapi_time[14:15] == ".":
            frac_len = len(hapi_time) - 15
            zero_pads = "".join(["0" for i in range(0, 6 - frac_len)])
            microsec = hapi_time[15:21] + zero_pads
        else:
            microsec = "000000"

        return "%04s-%02s-%02sT%02s%%3A%02s%%3A%02s.%s" % \
               (year, month, day, hour, min, sec, microsec)

    def __request(self, url, headers={}, use_token=True, data=None):
        if use_token:
            headers["X-Auth-Token"] = self.__token
        request = urllib2.Request(url, headers=headers, data=data)
        raw_response = urllib2.urlopen(request).read()
        return json.loads(raw_response)

    def __parse_alarm_host(self, threshold_rule):
        # TODO [long term]
        # we should handle the case many hosts are involved in the alarm.
        # HAPI2 handles only one host for every trigger.
        # To do solve this problem, we have to extend the HAPI specifiation.
        query_array = threshold_rule.get("query")
        if query_array is None:
            return "N/A", "N/A"

        for query in query_array:
            host_id = self.__parse_alarm_host_each(query)
            if host_id is not None:
                break
        else:
            return "N/A", "N/A"

        host_name = self.__host_cache.get(host_id, "N/A")
        return host_id, host_name

    def __parse_alarm_host_each(self, query):
        field = query.get("field")
        if field is None:
            return None
        value = query.get("value")
        if value is None:
            return None
        op = query.get("op")
        if value is None:
            return None
        if op != "eq":
            logger.info("Unknown eperator: %s" % op)
            return None
        return value

    def __fixup_event_last_info(self, last_info):
        if last_info is None:
            fixed = self.get_cached_event_last_info()
        else:
            fixed = last_info
        if fixed == self.INITIAL_LAST_INFO:
            fixed = None
        return fixed

    @classmethod
    def alarm_to_hapi_status(cls, alarm_type, detail_json):
        assert alarm_type == "creation" or alarm_type == "state transition"
        detail = json.loads(detail_json)
        state = detail["state"]
        return cls.STATUS_MAP[state]

    @classmethod
    def status_to_hapi_event_type(cls, hapi_status):
        return cls.STATUS_EVENT_MAP[hapi_status]

    @staticmethod
    def parse_time(time):
        """
        Parse time strings returned from OpenStack.

        @param time
        A time string such as
        - 2014-09-05T06:25:26.007000
        - 2014-09-05T06:25:26

        @return A datetime object.
        """

        EXPECT_LEN_WITHOUT_MICRO = 19
        EXPECT_LEN_WITH_MICRO = EXPECT_LEN_WITHOUT_MICRO + 7
        formats = {
            EXPECT_LEN_WITHOUT_MICRO: "%Y-%m-%dT%H:%M:%S",
            EXPECT_LEN_WITH_MICRO: "%Y-%m-%dT%H:%M:%S.%f",
        }
        return datetime.datetime.strptime(time, formats[len(time)])


class Hap2CeilometerPoller(haplib.BasePoller, Common):

    def __init__(self, *args, **kwargs):
        haplib.BasePoller.__init__(self, *args, **kwargs)
        Common.__init__(self)

    def poll_setup(self):
        self.ensure_connection()

    def poll_hosts(self):
        self.collect_hosts_and_put()

    def poll_host_groups(self):
        self.collect_host_groups_and_put()

    def poll_host_group_membership(self):
        self.collect_host_group_membership_and_put()

    def poll_triggers(self):
        self.collect_triggers_and_put()

    def poll_events(self):
        self.collect_events_and_put()

    def on_aborted_poll(self):
        self.reset()
        self.close_connection()


class Hap2CeilometerMain(haplib.BaseMainPlugin, Common):
    def __init__(self, *args, **kwargs):
        haplib.BaseMainPlugin.__init__(self)
        Common.__init__(self)

    def hap_fetch_triggers(self, params, request_id):
        self.ensure_connection()
        self.get_sender().response("SUCCESS", request_id)
        fetch_id = params["fetchId"]
        host_ids = params.get("hostIds")
        self.collect_triggers_and_put(fetch_id, host_ids)

    def hap_fetch_events(self, params, request_id):
        self.ensure_connection()
        self.get_sender().response("SUCCESS", request_id)
        self.collect_events_and_put(params["fetchId"], params["lastInfo"],
                                    params["count"], params["direction"])

    def hap_fetch_items(self, params, request_id):
        self.ensure_connection()
        self.get_sender().response("SUCCESS", request_id)
        self.collect_items_and_put(params["fetchId"], params.get("hostIds"))

    def hap_fetch_history(self, params, request_id):
        self.ensure_connection()
        self.get_sender().response("SUCCESS", request_id)
        self.collect_history_and_put(params["fetchId"],
                                     params["hostId"], params["itemId"],
                                     params["beginTime"], params["endTime"])


class Hap2Ceilometer(standardhap.StandardHap):
    def create_main_plugin(self, *args, **kwargs):
        return Hap2CeilometerMain(*args, **kwargs)

    def create_poller(self, *args, **kwargs):
        return Hap2CeilometerPoller(self, *args, **kwargs)


if __name__ == '__main__':
    Hap2Ceilometer().run()
