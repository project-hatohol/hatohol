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

import haplib
import multiprocessing
import transporter
import argparse
import logging
import json


class SimpleServer:

    DEFAULT_MS_INFO = {
        "extendedInfo": "exampleExtraInfo",
        "serverId": 1,
        "url": "http://example.com:80",
        "type": 0,
        "nickName": "exampleName",
        "userName": "Admin",
        "password": "examplePass",
        "pollingIntervalSec": 30,
        "retryIntervalSec": 10
    }

    def __init__(self, args, transporter_args):
        self.__args = args
        self.__sender = haplib.Sender(transporter_args)
        self.__rpc_queue = hap.MultiprocessingJoinableQueue()
        self.__dispatcher = haplib.Dispatcher(self.__rpc_queue)
        self.__dispatcher.daemonize()
        self.__last_info = {"event": None}

        self.__handler_map = {
            "exchangeProfile": self.__rpc_exchange_profile,
            "getMonitoringServerInfo": self.__rpc_get_monitoring_server_info,
            "putHosts": self.__rpc_put_hosts,
            "putHostGroups": self.__rpc_put_host_groups,
            "putHostGroupMembership": self.__rpc_put_host_group_membership,
            "putTriggers": self.__rpc_put_triggers,
            "putEvents": self.__rpc_put_events,
            "putItems": self.__rpc_put_items,
            "putHistory": self.__rpc_put_history,
            "getLastInfo": self.__rpc_get_last_info,
            "putArmInfo": self.__rpc_put_arm_info,
        }

        # launch receiver process
        dispatch_queue = self.__dispatcher.get_dispatch_queue()
        self.__receiver = haplib.Receiver(transporter_args, dispatch_queue,
                                          self.__handler_map.keys())
        self.__receiver.daemonize()

    def __call__(self):
        while True:
            pm = self.__rpc_queue.get()
            if pm.error_code is not None:
                logging.error(pm.get_error_message())
                continue
            msg = pm.message_dict
            method = msg.get("method")
            if method is None:
                if msg.get("result") is not None:
                    logging.info("Receive: %s" % msg)
                else:
                    logging.error("Unexpected message: %s" % msg)
                continue
            params = msg["params"]
            logging.info("method: %s" % method)
            call_id = msg["id"]
            self.__handler_map[method](call_id, params)

    def __rpc_exchange_profile(self, call_id, params):
        logging.info(params)
        # TODO: Parse content
        result = {"name": "SimpleServer",
                  "procedures": self.__handler_map.keys()}
        self.__sender.response(result, call_id)

    def __rpc_get_monitoring_server_info(self, call_id, params):
        file_name = self.__args.ms_info
        if file_name is not None:
            with open(file_name, "r") as file:
                result = json.load(file)
        else:
            result = self.DEFAULT_MS_INFO
        self.__sender.response(result, call_id)

    def __rpc_put_hosts(self, call_id, params):
        logging.info(params)
        # TODO: Parse content
        result = "SUCCESS"
        self.__sender.response(result, call_id)

    def __rpc_put_host_groups(self, call_id, params):
        logging.info(params)
        # TODO: Parse content
        result = "SUCCESS"
        self.__sender.response(result, call_id)

    def __rpc_put_host_group_membership(self, call_id, params):
        logging.info(params)
        # TODO: Parse content
        result = "SUCCESS"
        self.__sender.response(result, call_id)

    def __rpc_put_triggers(self, call_id, params):
        logging.info(params)
        # TODO: Parse content
        result = "SUCCESS"
        self.__sender.response(result, call_id)

    def __rpc_put_events(self, call_id, params):
        NUM_MAX_SHOW_EVENTS = 100

        events = params.get("events")
        num_events = len(events) if events is not None else 0
        last_info = params.get("lastInfo")
        msg = "num_events: %d, fetch_id: %s, mayMoreFlag: %s, lastIfno: %s" % (
              num_events, params.get("fetchId"), params.get("mayMoreFlag"),
              last_info)
        logging.info(msg)

        if num_events <= NUM_MAX_SHOW_EVENTS:
            logging.info(params)
        else:
            msg = "Skip to show events to suppress flooding."
            logging.info(msg)

        # TODO: Parse content
        if last_info is not None:
            self.__last_info["event"] = last_info
        result = "SUCCESS"
        self.__sender.response(result, call_id)

    def __rpc_put_items(self, call_id, params):
        logging.info(params)
        # TODO: Parse content
        result = "SUCCESS"
        self.__sender.response(result, call_id)

    def __rpc_put_history(self, call_id, params):
        logging.info(params)
        # TODO: Parse content
        result = "SUCCESS"
        self.__sender.response(result, call_id)

    def __rpc_get_last_info(self, call_id, params):
        logging.info(params)
        if not params in self.__last_info:
            logging.error("Invalid paramter: '%s'" % paramsa)
            self.__sender.error(haplib.ERR_CODE_INVALID_PARAMS, call_id)
            return
        result = self.__last_info[params]
        self.__sender.response(result, call_id)

    def __rpc_put_arm_info(self, call_id, params):
        logging.info(params)
        # TODO: Parse content
        result = "SUCCESS"
        self.__sender.response(result, call_id)


def basic_setup(arg_def_func=None, prog_name="Simple Server for HAPI 2.0"):
    logging.basicConfig(level=logging.INFO)
    logging.info(prog_name)
    parser = argparse.ArgumentParser()
    haplib.Utils.define_transporter_arguments(parser)
    if arg_def_func is not None:
        arg_def_func(parser)

    args = parser.parse_args()
    transporter_class = haplib.Utils.load_transporter(args)
    transporter_args = {"class": transporter_class}
    transporter_args.update(transporter_class.parse_arguments(args))
    transporter_args["amqp_send_queue_suffix"] = "-T"
    transporter_args["amqp_recv_queue_suffix"] = "-S"

    return args, transporter_args


def arg_def(parser):
    parser.add_argument("--ms-info",
                        help="A file including monitoring server info.")

if __name__ == '__main__':
    args, transporter_args = basic_setup(arg_def)
    server = SimpleServer(args, transporter_args)
    server()
