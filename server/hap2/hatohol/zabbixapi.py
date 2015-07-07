#! /usr/bin/env python
# coding: UTF-8
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

import urllib2
import json
import logging
from hatohol.haplib import Utils

TRIGGER_SEVERITY = {"-1": "ALL", "0": "UNKNOWN", "1": "INFO", "2": "WARNING",
                    "3": "ERROR", "4": "CRITICAL", "5": "EMERGENCY"}
TRIGGER_STATUS = {"0": "OK", "1": "NG", "2": "UNKNOWN"}
EVENT_TYPE = {"0": "GOOD", "1": "BAD", "2": "UNKNOWN", "3": "NOTIFICATION"}

class ZabbixAPI:
    def __init__(self, monitoring_server_info):
        self.url = monitoring_server_info.url
        self.auth_token = self.get_auth_token(monitoring_server_info.user_name,
                                              monitoring_server_info.password)
        self.api_version = self.get_api_version()

    def get_auth_token(self, user_name, user_passwd):
        params = {'user': user_name, 'password': user_passwd}
        res_dict = self.get_response_dict("user.authenticate", params)

        self.result = check_response(res_dict)
        if not self.result:
            logging.error("Authenticate failure: %s" % res_dict)
            return

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
        for item in res_dict["result"]:
            self.expand_item_brief(item)

            if int(item["lastns"]) == 0:
                ns = 0
            else:
                ns = Utils.translate_int_to_decimal(int(item["lastns"]))

            items.append({"itemId": item["itemid"],
                          "hostId": item["hostid"],
                          "brief": item["name"],
                          "lastValueTime": Utils.translate_unix_time_to_hatohol_time(int(item["lastclock"]) + ns),
                          "lastValue": item["lastvalue"],
                          "itemGroupName": get_item_groups(item["applications"]),
                          "unit": item["units"]})

        return items

    def expand_item_brief(self, item):
        while "$" in item["name"]:
            dollar_index = item["name"].find("$")
            key_index = int(item["name"][dollar_index+1: dollar_index+2])
            key_list_str = item["key_"][item["key_"].find("[")+1: -1]
            key_list = key_list_str.split(",")
            item["name"] = \
                item["name"].replace(item["name"][dollar_index: dollar_index+2],
                                     key_list[key_index-1])

    def get_history(self, item_id, begin_time, end_time):
        begin_time = Utils.translate_hatohol_time_to_unix_time(begin_time)
        end_time = Utils.translate_hatohol_time_to_unix_time(end_time)
        params = {"output": "extend", "itemids": item_id,
                  "history": self.get_item_value_type(item_id), "sortfield": "clock",
                  "sortorder": "ASC", "time_from": begin_time,
                  "time_till": end_time}
        res_dict = self.get_response_dict("history.get", params, self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return

        histories = list()
        for history in res_dict["result"]:
            if int(history["ns"]) == 0:
                ns = 0
            else:
                ns = Utils.translate_int_to_decimal(int(history["ns"]))

            histories.append({"value": history["value"],
                              "time": Utils.translate_unix_time_to_hatohol_time(int(history["clock"]) + ns)})

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
        for host_group in res_dict["result"]:
            host_groups.append({"groupId": host_group["groupid"],
                                "groupName": host_group["name"]})

        return host_groups

    def get_triggers(self, requestSince=None, host_ids=None):
        params = {"output": "extend", "selectHosts": ["name"], "active": True}
        last_change_since = int()
        if requestSince:
            last_change_since = \
                        Utils.translate_hatohol_time_to_unix_time(requestSince)
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
        for num, trigger in enumerate(res_dict["result"]):
            try:
                description = [ed["description"] for ed in
                               expanded_descriptions["result"]
                               if ed["triggerid"] == trigger["triggerid"]][0]
                triggers.append({"triggerId": trigger["triggerid"],
                                 "status": TRIGGER_STATUS[trigger["state"]],
                                 "severity": TRIGGER_SEVERITY[trigger["priority"]],
                                 "lastChangeTime": Utils.translate_unix_time_to_hatohol_time(int(trigger["lastchange"])),
                                 "hostId": trigger["hosts"][0]["hostid"],
                                 "hostName": trigger["hosts"][0]["name"],
                                 "brief": trigger["description"],
                                 "extendedInfo": description})
            except KeyError:
                logging.warning("Get a imperfect trigger: %s" % trigger)

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
        params = {"output": ["triggerid", "priority", "description", "state"],
                  "triggerids": [trigger_id], "expandDescription": 1}
        res_dict = self.get_response_dict("trigger.get", params,
                                          self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return
        return res_dict["result"][0]

    def get_events(self, event_id_from, event_id_till=None):
        params = {"output": "extend", "eventid_from": event_id_from,
                  "selectHosts": ["name"]}
        if event_id_till is not None:
            params["eventid_till"] = event_id_till

        res_dict = self.get_response_dict("event.get", params, self.auth_token)

        self.result = check_response(res_dict)
        if not self.result:
            return

        events = list()
        for event in res_dict["result"]:
            try:
                if int(event["ns"]) == 0:
                    ns = 0
                else:
                    ns = Utils.translate_int_to_decimal(int(event["ns"]))

                trigger = self.get_select_trigger(event["objectid"])
                events.append({"eventId": event["eventid"],
                               "time": Utils.translate_unix_time_to_hatohol_time(int(event["clock"]) + ns),
                               "type": EVENT_TYPE[event["value"]],
                               "triggerId": trigger["triggerid"],
                               "status": TRIGGER_STATUS[trigger["state"]],
                               "severity": TRIGGER_SEVERITY[trigger["priority"]],
                               "hostId": event["hosts"][0]["hostid"],
                               "hostName": event["hosts"][0]["name"],
                               "brief": trigger["description"],
                               "extendedInfo": ""})
            except KeyError:
                logging.warning("Get a imperfect event: %s" % event)

        return events

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


def check_response(response_dict):
    if "error" in response_dict:
        return False

    return True
