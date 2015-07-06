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

import daemon
import json
import multiprocessing
import Queue
import argparse
import time
from hatohol.haplib import Utils
from hatohol import haplib
from hatohol import zabbixapi
from hatohol import standardhap


class PreviousHostsInfo:
    def __init__(self):
        self.hosts = list()
        self.host_groups = list()
        self.host_group_membership = list()


class ZabbixAPIConductor:
    def __init__(self):
        self.__api = None
        self.__previous_hosts_info = PreviousHostsInfo()
        self.__trigger_last_info = None
        self.__component_code = self.get_component_code()


    def reset(self):
        self.__api = None

    def make_sure_token(self):
        if self.__api is not None:
            return
        ms_info = self.get_ms_info()
        assert ms_info is not None
        self.__api = zabbixapi.ZabbixAPI(ms_info)

    def request(self, procedure_name, params):
        raise NotImplementedError

    def _wait_acknowledge(self, request_id):
        raise NotImplementedError

    def _wait_response(self, request_id):
        raise NotImplementedError

    def get_component_code(self):
        raise NotImplementedError

    def get_sender(self, request_id):
        raise NotImplementedError

    def collect_and_put_items(self, host_ids=None, fetch_id=None):
        self.put_items(self.__api.get_items(host_ids), fetch_id)

    def collect_and_put_history(self, item_id, begin_time, end_time, fetch_id):
        self.put_history(self.__api.get_history(item_id, begin_time, end_time),
                         item_id, fetch_id)

    def update_hosts_and_host_group_membership(self):
        hosts, hg_membership = self.__api.get_hosts()
        self.put_hosts(hosts)
        self.put_host_group_membership(hg_membership)

    def update_host_groups(self):
        host_groups = self.__api.get_host_groups()
        self.put_host_groups(host_groups)

    def update_triggers(self, host_ids=None, fetch_id=None):
        if self.__trigger_last_info is None:
            self.__trigger_last_info = self.get_last_info("trigger")

        triggers = self.__api.get_triggers(self.__trigger_last_info, host_ids)
        if not len(triggers):
            return

        self.__trigger_last_info = \
            Utils.get_biggest_num_of_dict_array(triggers,
                                                "lastChangeTime")
        update_type = "ALL" if fetch_id is not None else "UPDATED"

        self.put_triggers(triggers, update_type=update_type,
                          last_info=self.__trigger_last_info,
                          fetch_id=fetch_id)

    def update_events(self, last_info=None, count=None, direction="ASC",
                      fetch_id=None):
        if last_info is None:
            last_info = self.get_cached_event_last_info()

        if direction == "ASC":
            event_id_from = last_info
            event_id_till = None
            if count is not None:
                event_id_till = event_id_from + count
        # The following elif sentence is used from only fetchEvents
        elif direction == "DESC":
            event_id_till = last_info
            event_id_from = event_id_till - count

        events = self.__api.get_events(event_id_from, event_id_till)
        if len(events) == 0:
            return

        self.put_events(events, fetch_id)


class Hap2ZabbixAPIPoller(haplib.BasePoller, ZabbixAPIConductor):
    def __init__(self, *args, **kwargs):
        haplib.BasePoller.__init__(self, *args, **kwargs)
        ZabbixAPIConductor.__init__(self)

    # @override
    def poll(self):
        self.make_sure_token()
        self.update_hosts_and_host_group_membership()
        self.update_host_groups()
        self.update_triggers()
        self.update_events()

class Hap2ZabbixAPIMain(haplib.BaseMainPlugin, ZabbixAPIConductor):
    def __init__(self, *args, **kwargs):
        haplib.BaseMainPlugin.__init__(self)
        ZabbixAPIConductor.__init__(self)

    def hap_fetch_items(self, params, request_id):
        self.make_sure_token()
        self.get_sender().response("SUCCESS", request_id)
        self.collect_and_put_items(params.get("hostIds"), params["fetchId"])

    def hap_fetch_history(self, params, request_id):
        self.make_sure_token()
        self.get_sender().response("SUCCESS", request_id)
        self.collect_and_put_history(params["itemId"], params["beginTime"],
                                     params["endTime"], params["fetchId"])

    def hap_fetch_triggers(self, params, request_id):
        self.make_sure_token()
        self.get_sender().response("SUCCESS", request_id)
        self.update_triggers(params.get("hostIds"), params["fetchId"])

    def hap_fetch_events(self, params, request_id):
        self.make_sure_token()
        self.get_sender().response("SUCCESS", request_id)
        self.update_events(params["lastInfo"], params["count"],
                           params["direction"], params["fetchId"])


class Hap2ZabbixAPI(standardhap.StandardHap):
    def create_main_plugin(self, *args, **kwargs):
        return Hap2ZabbixAPIMain(*args, **kwargs)

    def create_poller(self, *args, **kwargs):
        return Hap2ZabbixAPIPoller(self, *args, **kwargs)


if __name__ == '__main__':
    hap = Hap2ZabbixAPI()
    hap()
