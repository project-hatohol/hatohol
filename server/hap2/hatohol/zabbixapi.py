#! /usr/bin/env python
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
from logging import getLogger
from hatohol import hap
from hatohol import hapcommon

logger = getLogger("hatohol.zabbixapi:%s" % hapcommon.get_top_file_name())

TRIGGER_SEVERITY = {"-1": "ALL", "0": "UNKNOWN", "1": "INFO", "2": "WARNING",
                    "3": "ERROR", "4": "CRITICAL", "5": "EMERGENCY"}
TRIGGER_STATUS = {"0": "OK", "1": "NG", "2": "UNKNOWN"}
EVENT_TYPE = {"0": "GOOD", "1": "BAD", "2": "UNKNOWN", "3": "NOTIFICATION"}


class AuthenticationError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
         return repr(self.value)


class ZabbixAPI:
    def __init__(self, monitoring_server_info):
        self.url = monitoring_server_info.url
        self.auth_token = self.get_auth_token(monitoring_server_info.user_name,
                                              monitoring_server_info.password)
        self.api_version = self.get_api_version()

    @staticmethod
    def __iterate_in_try_block(seq, func, *args, **kwargs):
        for s in seq:
            try:
                func(s, *args, **kwargs)
            except:
                logger.warning("Failed to process element: %s" % s)
                hap.handle_exception()

    def get_auth_token(self, user_name, user_passwd):
        params = {'user': user_name, 'password': user_passwd}
        res_dict = self.get_response_dict("user.login", params)

        self.result = check_response(res_dict)
        if not self.result:
            logger.error("Authentication failure: %s" % res_dict)
            raise AuthenticationError("Authentication failure: %s" % res_dict)

        return res_dict["result"]

    def get_api_version(self):
        res_dict = self.get_response_dict("apiinfo.version", None)

        self.result = check_response(res_dict)
        if not self.result:
            return

        return res_dict["result"][0:3]

    def get_items(self, host_ids=None):
        params = {"output": "extend", "selectApplications": ["name"],
                  "monitored": True}
        if host_ids is not None:
            params["hostids"] = host_ids

        res_dict = self.get_response_dict("item.get", params, self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return

        items = list()
        def proc(item):
            if item["lastclock"] == "0":
                return
            if "$" in item["name"]:
                self.expand_item_brief(item)

            time = \
                hapcommon.translate_unix_time_to_hatohol_time(item["lastclock"],
                                                              item["lastns"])
            items.append({"itemId": item["itemid"],
                          "hostId": item["hostid"],
                          "brief": item["name"],
                          "lastValueTime": time,
                          "lastValue": item["lastvalue"],
                          "itemGroupName": get_item_groups(item["applications"]),
                          "unit": item["units"]})

        self.__iterate_in_try_block(res_dict["result"], proc)
        return items

    def expand_item_brief(self, item):
        key_list = item["key_"][item["key_"].find("[")+1: -1].split(",")
        separated_item_list = item["name"].split("$")
        item["name"] = separated_item_list[0]

        for separated_item in separated_item_list[1:]:
            if not separated_item: continue

            key_index = separated_item[0]
            if key_index.isdigit():
                item["name"] += key_list[int(key_index)-1]
            else:
                item["name"] += "$"
            item["name"] += separated_item[1:]

    def get_history(self, item_id, begin_time, end_time):
        begin_time = hapcommon.translate_hatohol_time_to_unix_time(begin_time)
        end_time = hapcommon.translate_hatohol_time_to_unix_time(end_time)
        params = {"output": "extend", "itemids": item_id,
                  "history": self.get_item_value_type(item_id), "sortfield": "clock",
                  "sortorder": "ASC", "time_from": begin_time,
                  "time_till": end_time}
        res_dict = self.get_response_dict("history.get", params, self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return

        histories = list()
        def proc(history):
            time = \
                hapcommon.translate_unix_time_to_hatohol_time(history["clock"],
                                                              history["ns"])
            histories.append({"value": history["value"], "time": time})
        self.__iterate_in_try_block(res_dict["result"], proc)

        return histories

    def get_item_value_type(self, item_id):
        params = {"output": ["value_type"], "itemids": [item_id]}
        res_dict = self.get_response_dict("item.get", params, self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return

        return res_dict["result"][0]["value_type"]

    # The following method gets not only hosts info but also host group membership.
    def get_hosts(self):
        params = {"output": "extend", "selectGroups": "refer",
                  "monitored_hosts": True}
        res_dict = self.get_response_dict("host.get", params, self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return

        hosts = list()
        host_group_membership = list()
        for host in res_dict["result"]:
            hosts.append({"hostId": host["hostid"], "hostName": host["name"]})

            group_ids = list()
            for groups in host["groups"]:
                group_ids.append(groups["groupid"])

            host_group_membership.append({"hostId": host["hostid"],
                                          "groupIds": group_ids})

        return (hosts, host_group_membership)

    def get_host_groups(self):
        params = {"output": "extend", "real_hosts": True, "monitored_hosts": True}
        res_dict = self.get_response_dict("hostgroup.get", params,
                                          self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return

        host_groups = list()
        def proc(host_group):
            host_groups.append({"groupId": host_group["groupid"],
                                "groupName": host_group["name"]})

        self.__iterate_in_try_block(res_dict["result"], proc)
        return host_groups

    def get_triggers(self, requestSince=None, host_ids=None):
        params = {"output": "extend", "selectHosts": ["name"], "active": True}
        last_change_since = int()
        if requestSince:
            last_change_since = \
                hapcommon.translate_hatohol_time_to_unix_time(requestSince)
            params["lastChangeSince"] = last_change_since
        if host_ids is not None:
            params["hostids"] = host_ids

        res_dict = self.get_response_dict("trigger.get", params,
                                          self.auth_token)
        expanded_descriptions = \
            self.get_trigger_expand_description(last_change_since, host_ids)

        self.result = check_response(res_dict)
        if not self.result:
            return

        triggers = list()
        def find_description(triggerid):
            for ex_descr in expanded_descriptions["result"]:
                if ex_descr["triggerid"] == triggerid:
                    return ex_descr["description"]
            logger.warning("Not found description: triggerid: %s" % triggerid)
            return ""

        def proc(trigger):
            description = find_description(trigger["triggerid"])
            lastchange = trigger["lastchange"]
            time = hapcommon.translate_unix_time_to_hatohol_time(lastchange)
            triggers.append({"triggerId": trigger["triggerid"],
                             "status": TRIGGER_STATUS[trigger["value"]],
                             "severity": TRIGGER_SEVERITY[trigger["priority"]],
                             "lastChangeTime": time,
                             "hostId": trigger["hosts"][0]["hostid"],
                             "hostName": trigger["hosts"][0]["name"],
                             "brief": trigger["description"],
                             "extendedInfo": description})

        self.__iterate_in_try_block(res_dict["result"], proc)
        return triggers

    def get_trigger_expand_description(self, last_change_since=None,
                                       host_ids=None):
        params = {"output": ["description"], "expandDescription": 1,
                  "active": True}
        if last_change_since:
            params["lastChangeSince"] = int(last_change_since)
        if host_ids is not None:
            params["hostids"] = host_ids

        res_dict = self.get_response_dict("trigger.get", params, self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return

        return res_dict

    def get_select_trigger(self, trigger_id):
        params = {"output": ["triggerid", "priority", "description", "value"],
                  "triggerids": [trigger_id], "expandDescription": 1,
                  "selectHosts": ["name"]}
        res_dict = self.get_response_dict("trigger.get", params,
                                          self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return
        return res_dict["result"][0]

    def get_events(self, event_id_from, event_id_till=None):
        params = {"output": "extend", "eventid_from": event_id_from}
        if event_id_till is not None:
            params["eventid_till"] = event_id_till

        res_dict = self.get_response_dict("event.get", params, self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return

        events = list()
        def proc(event):
            trigger = self.get_select_trigger(event["objectid"])
            time = \
                hapcommon.translate_unix_time_to_hatohol_time(event["clock"],
                                                              event["ns"])
            events.append({"eventId": event["eventid"],
                           "time": time,
                           "type": EVENT_TYPE[event["value"]],
                           "triggerId": trigger["triggerid"],
                           "status": TRIGGER_STATUS[event["value"]],
                           "severity": TRIGGER_SEVERITY[trigger["priority"]],
                           "hostId": trigger["hosts"][0]["hostid"],
                           "hostName": trigger["hosts"][0]["name"],
                           "brief": trigger["description"],
                           "extendedInfo": ""})

        self.__iterate_in_try_block(res_dict["result"], proc)
        return events


    def get_event_end_id(self, is_first=False):
        params = {"output": "shorten", "sortfield": "eventid", "limit": 1}
        if is_first:
            params["sortorder"] = "ASC"
        else:
            params["sortorder"] = "DESC"

        res_dict = self.get_response_dict("event.get", params, self.auth_token)

        self.result = check_response(res_dict, 1)
        if not self.result:
            return

        return res_dict["result"][0]["eventid"]


    def get_response_dict(self, method_name, params, auth_token=None):
        HEADER = {"Content-Type": "application/json-rpc"}
        post = json.dumps({"jsonrpc": "2.0", "method": method_name,
                           "params": params, "auth": auth_token, "id": 1})
        request = urllib2.Request(self.url, post, HEADER)
        response = urllib2.urlopen(request)
        res_str = response.read()

        return json.loads(res_str)


def get_item_groups(applications):
    item_groups = list()
    for application in applications:
        item_groups.append(application["name"])

    return item_groups


def check_response(response_dict, expected_minimum_result_length=0):
    if "error" in response_dict:
        logger.warning("Response dictionary contains an error: %s" % response_dict)
        return False

    if len(response_dict["result"]) < expected_minimum_result_length:
        logger.warning("Response dictionary result is less than expected length: %s" % response_dict)
        return False

    return True
