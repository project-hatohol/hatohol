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

import time
import datetime
import socket
import uuid
import json
from logging import getLogger
from mk_livestatus import Socket
from hatohol import hap
from hatohol import haplib
from hatohol import hapcommon
from hatohol import standardhap

logger = getLogger("hatohol.hap2_nagios_livestatus:%s" % hapcommon.get_top_file_name())


class Common:

    STATE_OK = 0
    STATE_WARNING = 1
    STATE_CRITICAL = 2

    STATUS_MAP = {STATE_OK: "OK", STATE_WARNING: "NG", STATE_CRITICAL: "NG"}
    SEVERITY_MAP = {
        STATE_OK: "INFO", STATE_WARNING: "WARNING", STATE_CRITICAL: "CRITICAL"}
    EVENT_TYPE_MAP = {
        STATE_OK: "GOOD", STATE_WARNING: "BAD", STATE_CRITICAL: "BAD"}

    DEFAULT_PATH = "/tmp/nagios.sock"
    INITIAL_LAST_INFO = ""

    def __init__(self):
        self.__access_point = self.DEFAULT_PATH
        self.__port = None
        self.__time_offset = datetime.timedelta(seconds=time.timezone)
        self.__trigger_last_info = None
        self.__latest_statehist = None

    def ensure_connection(self):
        # load MonitoringServerInfo
        ms_info = self.get_ms_info()
        if ms_info is None:
            logger.info("Use default connection parameters.")
        else:
            self.__access_point, self.__port = self.__parse_url(ms_info.url)

        logger.info("Try to connection: Sv: %s" % self.__access_point)

        socket_arg = self.__access_point
        try:
            if self.__port:
                socket_arg = (socket_arg, self.__port)
            self.__socket = Socket(socket_arg)
            test_query = self.__socket.contacts
            test_query.call()
        except socket.error as (errno, msg):
            logger.error('Socket Error [%d]: %s' % (errno, msg))
            raise hap.Signal

    def __parse_url(self, url):
        # [URL] "SERVER_IP:PORT" or "PATH"
        colon_idx = url.find(":")
        if colon_idx == -1:
            path = url
            return path, None
        else:
            port = int(url[colon_idx+1:])
            server = url[0:colon_idx]
            return server, port

    def collect_hosts_and_put(self):
        query = self.__socket.hosts.columns("name", "alias")
        result = query.call()
        hosts = [{"hostId": host["name"], "hostName": host["alias"]} for host in result]
        self.divide_and_put_data(self.put_hosts, hosts)

    def collect_host_groups_and_put(self):
        query = self.__socket.hostgroups.columns("name", "alias")
        result = query.call()
        groups = \
            [{"groupId": group["name"], "groupName": group["alias"]} for group in result]
        self.divide_and_put_data(self.put_host_groups, groups)

    def collect_host_group_membership_and_put(self):
        query = self.__socket.hosts.columns("name", "groups")
        result = query.call()
        membership = [{"hostId": host["name"], "groupIds": host["groups"]} for host in result]

        self.divide_and_put_data(self.put_host_group_membership, membership)

    def collect_triggers_and_put(self, fetch_id=None, host_ids=None):
        query = self.__socket.services.columns("description",
                                               "last_state_change",
                                               "host_alias",
                                               "host_name",
                                               "state")

        if host_ids is not None:
            filter_condition = "host_name ~ "
            for host_id in enumerate(host_ids):
                if host_id[0] == 0:
                    filter_condition += host_id[1]
                else:
                    filter_condition += "|" + host_id[1]

            query = query.filter(filter_condition)

        all_triggers_should_send = lambda: fetch_id is None
        update_type = "ALL"
        if all_triggers_should_send():
            if self.__trigger_last_info is None:
                self.__trigger_last_info = self.get_last_info("trigger")

            if len(self.__trigger_last_info):
                unix_timestamp = hapcommon.translate_hatohol_time_to_unix_time(self.__trigger_last_info)
                query = query.filter("last_state_change > %d" % unix_timestamp)
                update_type = "UPDATED"

        result = query.call()

        triggers = []
        for service in result:
            hapi_status, hapi_severity = \
                self.__parse_status_and_severity(service["state"])

            last_state_change = datetime.datetime.fromtimestamp(service["last_state_change"])
            hapi_time = hapcommon.conv_to_hapi_time(last_state_change,
                                                    self.__time_offset)
            triggers.append({
                "triggerId": service["description"],
                "status": hapi_status,
                "severity": hapi_severity,
                "lastChangeTime": hapi_time,
                "hostId": service["host_name"],
                "hostName": service["host_alias"],
                "brief": service["description"],
                "extendedInfo": ""
            })
        self.__trigger_last_info = \
            hapcommon.get_biggest_num_of_dict_array(triggers,
                                                    "lastChangeTime")

        put_empty_contents = True
        if fetch_id is None:
            put_empty_contents = False

        self.divide_and_put_data(self.put_triggers, triggers,
                           put_empty_contents,
                           update_type=update_type,
                           last_info=self.__trigger_last_info,
                           fetch_id=fetch_id)

    def collect_events_and_put(self, fetch_id=None, last_info=None,
                               count=None, direction="ASC"):
        query = self.__socket.statehist.columns("log_output",
                                                "state",
                                                "time",
                                                "current_host_name",
                                                "current_host_alias",
                                                "service_description")
        if last_info is None:
            last_info = self.get_cached_event_last_info()
        if not last_info:
            last_info = datetime.datetime.now().strftime("%s")
            self.set_event_last_info(last_info)

        if direction == "ASC":
            query = query.filter("time > %s" % last_info)
        elif direction == "DESC":
            query = query.filter("time < %s" % last_info)
        else:
            logger.error("Set unknow direction: %s" % direction);
            return

        result = query.call()
        logger.debug(query)
        logger.debug(result)

        events = []
        for event in result:
            if not len(event["current_host_name"]):
                continue

            hapi_event_type = self.EVENT_TYPE_MAP.get(event["state"])
            if hapi_event_type is None:
                logger.warning("Unknown status: %d" % event["state"])
                hapi_event_type = "UNKNOWN"

            hapi_status, hapi_severity = \
                self.__parse_status_and_severity(event["state"])

            event_time = datetime.datetime.fromtimestamp(event["time"])
            hapi_time = hapcommon.conv_to_hapi_time(event_time,
                                                    self.__time_offset)
            events.append({
                "eventId": str(uuid.uuid1()),
                "time": hapi_time,
                "type": hapi_event_type,
                "triggerId": event["service_description"],
                "status": hapi_status,
                "severity": hapi_severity,
                "hostId": event["current_host_name"],
                "hostName": event["current_host_alias"],
                "brief": event["log_output"],
                "extendedInfo": ""
            })

        if len(result):
            # livestatus return a sorted list.
            # result[0] is latest statehist.
            self.__latest_statehist = str(result[0]["time"])

        put_empty_contents = True
        if fetch_id is None:
            put_empty_contents = False

        self.divide_and_put_data(self.put_events, events, put_empty_contents,
                           fetch_id=fetch_id,
                           last_info_generator=self.return_latest_statehist)

    def return_latest_statehist(self, events):
        # livestatus plugins does not use events.
        # But, haplib.put_events last_info_generator have wanted events.
        return self.__latest_statehist

    def __parse_status_and_severity(self, status):
        hapi_status = self.STATUS_MAP.get(status)
        if hapi_status is None:
            logger.warning("Unknown status: " + str(status))
            hapi_status = "UNKNOWN"

        hapi_severity = self.SEVERITY_MAP.get(status)
        if hapi_severity is None:
            logger.warning("Unknown status: " + str(status))
            hapi_severity = "UNKNOWN"

        return (hapi_status, hapi_severity)


class Hap2NagiosLivestatusPoller(haplib.BasePoller, Common):

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


class Hap2NagiosLivestatusMain(haplib.BaseMainPlugin, Common):
    def __init__(self, *args, **kwargs):
        haplib.BaseMainPlugin.__init__(self)
        Common.__init__(self)

    def hap_fetch_triggers(self, params, request_id):
        self.ensure_connection()
        # TODO: return FAILURE when connection fails
        self.get_sender().response("SUCCESS", request_id)
        fetch_id = params["fetchId"]
        host_ids = params.get("hostIds")
        self.collect_triggers_and_put(fetch_id, host_ids)

    def hap_fetch_events(self, params, request_id):
        self.ensure_connection()
        # TODO: return FAILURE when connection fails
        self.get_sender().response("SUCCESS", request_id)
        self.collect_events_and_put(params["fetchId"], params["lastInfo"],
                                    params["count"], params["direction"])


class Hap2NagiosLivestatus(standardhap.StandardHap):
    def create_main_plugin(self, *args, **kwargs):
        return Hap2NagiosLivestatusMain(*args, **kwargs)

    def create_poller(self, *args, **kwargs):
        return Hap2NagiosLivestatusPoller(self, *args, **kwargs)


if __name__ == '__main__':
    Hap2NagiosLivestatus().run()
