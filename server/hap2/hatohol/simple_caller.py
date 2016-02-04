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

import haplib
import simple_server
import logging
import json

class SimpleCaller:

    def __init__(self, transporter_args):
        self.__sender = haplib.Sender(transporter_args)
        self.__COMMAND_HANDLERS = {
            "exchangeProfile": self.__rpc_exchange_profile,
            "fetchTriggers": self.__rpc_fetch_triggers,
            "fetchEvents":   self.__rpc_fetch_events,
            "fetchItems":    self.__rpc_fetch_items,
            "fetchHistory":  self.__rpc_fetch_history,
            "updateMonitoringServerInfo":
            self.__rpc_notify_monitoring_server_info,
        }

    def __call__(self, args):
        logging.info("Command: %s" % args.command)
        self.__curr_command = args.command
        self.__COMMAND_HANDLERS[args.command](args)
        self.__curr_command = None

    def __rpc_exchange_profile(self, args):
        params = {"name": "SimpleCaller", "procedures": ["exchangeProfile"]}
        self.__request(params)

    def __rpc_fetch_triggers(self, args):
        params = {"hostIds": args.host_ids, "fetchId": args.fetch_id}
        self.__request(params)

    def __rpc_fetch_events(self, args):
        params = {"lastInfo": args.last_info, "count": args.count,
                  "direction": args.direction, "fetchId": args.fetch_id}
        self.__request(params)

    def __rpc_fetch_items(self, args):
        params = {"fetchId": args.fetch_id, "hostIds": args.host_ids}
        self.__request(params)

    def __rpc_fetch_history(self, args):
        params = {"fetchId": args.fetch_id,
                  "hostId": args.host_id, "itemId": args.item_id,
                  "beginTime": args.begin_time, "endTime": args.end_time}
        self.__request(params)

    def __rpc_notify_monitoring_server_info(self, args):
        file_name = args.ms_info
        with open(file_name, "r") as file:
            params = json.load(file)
        self.__notify(params)

    def __request(self, params):
        __component_code = 0
        request_id = haplib.Utils.generate_request_id(__component_code)
        self.__sender.request(self.__curr_command, params, request_id)

    def __notify(self, params):
        self.__sender.request(self.__curr_command, params, request_id=None)

    @staticmethod
    def arg_def(parser):
        subparsers = parser.add_subparsers(dest='command',
                                           help='sub-command help')

        parser_fetch_trig = subparsers.add_parser('exchangeProfile')

        parser_fetch_trig = subparsers.add_parser('fetchTriggers')
        parser_fetch_trig.add_argument('--host-ids', nargs="+", default=["1"])
        parser_fetch_trig.add_argument('--fetch-id', default="1")

        parser_fetch_evt = subparsers.add_parser('fetchEvents')
        parser_fetch_evt.add_argument('--last-info', default="")
        parser_fetch_evt.add_argument('--count', type=int, default=1000)
        parser_fetch_evt.add_argument('--direction', choices=["ASC", "DESC"],
                                      default="ASC")
        parser_fetch_evt.add_argument('--fetch-id', default="1")

        parser_update_msi = subparsers.add_parser('updateMonitoringServerInfo')
        parser_update_msi.add_argument('ms_info')

        parser_fetch_item = subparsers.add_parser('fetchItems')
        parser_fetch_item.add_argument('--host-ids', nargs="+", required=True)
        parser_fetch_item.add_argument('--fetch-id', default="1")

        parser_fetch_hist = subparsers.add_parser('fetchHistory')
        parser_fetch_hist.add_argument('--host-id', required=True)
        parser_fetch_hist.add_argument('--item-id', required=True)
        parser_fetch_hist.add_argument('--begin-time', required=True)
        parser_fetch_hist.add_argument('--end-time', required=True)
        parser_fetch_hist.add_argument('--fetch-id', default="1")


if __name__ == '__main__':
    prog_name = "Simple Caller for HAPI 2.0"
    args, transporter_args = simple_server.basic_setup(SimpleCaller.arg_def,
                                                       prog_name=prog_name)
    caller = SimpleCaller(transporter_args)
    caller(args)
