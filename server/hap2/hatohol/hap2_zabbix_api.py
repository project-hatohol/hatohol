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

import time
from logging import getLogger
from hatohol import hapcommon
from hatohol import haplib
from hatohol import zabbixapi
from hatohol import standardhap

logger = getLogger("hatohol.hap2_zabbix_api:%s" % hapcommon.get_top_file_name())
MAX_NUMBER_OF_EVENTS_FROM_ZABBIX = 1000
MAX_NUMBER_OF_EVENTS_OF_HAPI2 = 1000


class ZabbixAPIConductor(object):
    def __init__(self):
        self.__api = None
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
        self.divide_and_put_data(self.put_items, self.__api.get_items(host_ids),
                                 True, fetch_id)

    def collect_and_put_history(self, item_id, begin_time, end_time, fetch_id):
        history = self.__api.get_history(item_id, begin_time, end_time)
        self.divide_and_put_data(self.put_history, history, True,
                                 item_id, fetch_id)

    def update_hosts_and_host_group_membership(self):
        hosts, hg_membership = self.__api.get_hosts()
        self.divide_and_put_data(self.put_hosts, hosts)
        self.divide_and_put_data(self.put_host_group_membership, hg_membership)

    def update_host_groups(self):
        host_groups = self.__api.get_host_groups()
        self.divide_and_put_data(self.put_host_groups, host_groups)

    def update_triggers(self, host_ids=None, fetch_id=None):
        if self.__trigger_last_info is None:
            self.__trigger_last_info = self.get_last_info("trigger")

        put_empty_contents=False
        if fetch_id is not None:
            update_type = "ALL"
            triggers = self.__api.get_triggers(host_ids=host_ids)
            put_empty_contents=True
        else:
            update_type = "UPDATED"
            triggers = self.__api.get_triggers(self.__trigger_last_info, host_ids)

        if not len(triggers):
            return

        self.__trigger_last_info = \
            hapcommon.get_biggest_num_of_dict_array(triggers,
                                                    "lastChangeTime")

        self.divide_and_put_data(self.put_triggers, triggers,
                           put_empty_contents,
                           update_type=update_type,
                           last_info=self.__trigger_last_info,
                           fetch_id=fetch_id)

    def update_events_poll(self):
        last_event_id = self.__api.get_event_end_id()
        event_ids = list()
        last_info = self.get_cached_event_last_info()
        if len(last_info):
            last_info = int(last_info)
        # If Hatohol server and this plugin does not have last_info,
        # set last_event_id to last_info.
        else:
            self.set_event_last_info(last_event_id)
            return

        last_event_id = int(last_event_id)
        while True:
            event_id_from = last_info + 1
            event_id_till = last_info + MAX_NUMBER_OF_EVENTS_FROM_ZABBIX
            event_ids.append((event_id_from, event_id_till))
            if event_id_till >= last_event_id: break
            last_info = event_id_till

        events = list()
        for event_from_id, event_till_id in event_ids:
            events = self.__api.get_events(event_from_id, event_till_id)
            if len(events) == 0:
                continue
            self.divide_and_put_data(self.put_events, events)

    def update_events_fetch(self, last_info, count, direction, fetch_id):
        if direction == "ASC":
            event_id_from = last_info + 1
            event_id_till = last_info + count
        elif direction == "DESC":
            event_id_from = last_info - count + 1
            event_id_till = last_info

        events = self.__api.get_events(event_id_from, event_id_till)

        if len(events) == 0:
            return

        self.divide_and_put_data(self.put_events, events,
                                 True, fetch_id=fetch_id)


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
        self.update_events_poll()

    # @override
    def on_aborted_poll(self):
        logger.error("Polling: aborted.")
        ZabbixAPIConductor.reset(self)

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
        count = params["count"]
        if count > MAX_NUMBER_OF_EVENTS_OF_HAPI2:
            error_msg = "%d count exceeds the limit of the HAPI2 specification." % count
            logger.error(error_msg)
            self.hap_return_error(error_msg, request_id)
            return

        self.make_sure_token()
        self.get_sender().response("SUCCESS", request_id)
        self.update_events_fetch(int(params["lastInfo"]), count,
                                 params["direction"], params["fetchId"])


class Hap2ZabbixAPI(standardhap.StandardHap):
    def create_main_plugin(self, *args, **kwargs):
        return Hap2ZabbixAPIMain(*args, **kwargs)

    def create_poller(self, *args, **kwargs):
        return Hap2ZabbixAPIPoller(self, *args, **kwargs)


if __name__ == '__main__':
    Hap2ZabbixAPI().run();
