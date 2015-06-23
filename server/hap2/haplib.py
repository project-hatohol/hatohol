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

import sys
import time
import logging
import traceback
import multiprocessing
import Queue
import json
from datetime import datetime
import random
import argparse
import imp
import transporter
from rabbitmqconnector import RabbitMQConnector

SERVER_PROCEDURES = {"exchangeProfile": True,
                     "getMonitoringServerInfo": True,
                     "getLastInfo": True,
                     "putItems": True,
                     "putHistory": True,
                     "putHosts": True,
                     "putHostGroups": True,
                     "putHostGroupMembership": True,
                     "updateTriggers": True,
                     "updateEvents": True,
                     "updateHostParent": True,
                     "putArmInfo": True}

PROCEDURES_DEFS = {
    "exchangeProfile": {
        "args": {
            "procedures": {"type": list(), "mandatory": True},
            "name": {"type": unicode(), "mandatory": True},
        }
    },
    "fetchItems": {
        "args": {
            "hostIds": {"type": list(), "mandatory": False},
            "fetchId": {"type": unicode(), "mandatory": True},
        }
    },
    "fetchHistory": {
        "args": {
            "hostId": {"type": unicode(), "mandatory": True},
            "itemId": {"type": unicode, "mandatory": True},
            "beginTime": {"type": unicode(), "mandatory": True},
            "endTime": {"type": unicode(), "mandatory": True},
            "fetchId": {"type": unicode(), "mandatory": True},
        }
    },
    "fetchTriggers": {
        "args": {
            "hostIds": {"type": list(), "mandatory": False},
            "fetchId": {"type": unicode(), "mandatory": True}
        }
    },
    "fetchEvents": {
        "args": {
            "lastInfo": {"type": unicode(), "mandatory": True},
            "count": {"type": int(), "mandatory": True},
            # TODO: validate: direction
            "direction": {"type": unicode(), "mandatory": True},
            "fetchId": {"type": unicode(), "mandatory": True}
        }
    },
    "notifyMonitoringServerInfo": {
        "notification": True,
        "args": {}  # TODO: fill content
    },
    "getMonitoringServerInfo": {
        "args": {}
    },
    "putHosts": {
        "args": {
            "hosts": {"type": list(), "mandatory": True}
        }
    },
    "putHostGroups": {
        "args": {
            "hostGroups": {"type": list(), "mandatory": True}
        }
    },
    "putHostGroupMembership": {
        "args": {
            "hostGroupMembership": {"type": list(), "mandatory": True}
        }
    },
    "putTriggers": {
        "args": {
            "triggers": {"type": list(), "mandatory": True},
            "updateType": {"type": unicode(), "mandatory": True}
        }
    },
    "putEvents": {
        "args": {
            "events": {"type": list(), "mandatory": True}
        }
    },
    "getLastInfo": {
        "args": {}
    },
    "putArmInfo": {
        "args": {}
    },
}


ERR_CODE_INVALID_REQUEST = -32600
ERR_CODE_METHOD_NOT_FOUND = -32601
ERR_CODE_INVALID_PARAMS = -32602
ERR_CODE_PARSER_ERROR = -32700
ERROR_DICT = {
    ERR_CODE_INVALID_REQUEST: "invalid Request",
    ERR_CODE_METHOD_NOT_FOUND: "Method not found",
    ERR_CODE_INVALID_PARAMS: "invalid params",
    ERR_CODE_PARSER_ERROR: "Parse error",
}

MAX_EVENT_CHUNK_SIZE = 1000


def handle_exception(raises=()):
    """
    Logging exception information including back trace and return
    some information. This method is supposed to be used in 'except:' block.
    Note that if the exception class is Signal, this method doesn't log it.

    @raises
    A sequence of exceptionclass names. If the handling exception is one of
    it, this method just raises it again.

    @return
    A sequence of exception class and the instance of the handling exception.
    """
    (exctype, value, tb) = sys.exc_info()
    if exctype in raises:
        raise
    if exctype is not Signal:
        logging.error("Unexpected error: %s, %s, %s" % \
                      (exctype, value, traceback.format_tb(tb)))
    return exctype, value


class Signal:
    """
    This class is supposed to raise as an exception in order to
    propagate some events and jump over stack frames.
    """

    def __init__(self, restart=False):
        self.restart = restart


class Callback:
    def __init__(self):
        self.__handlers = {}

    def register(self, code, handler):
        handler_list = self.__handlers.get(code)
        if handler_list is None:
            handler_list = []
            self.__handlers[code] = handler_list
        handler_list.append(handler)

    def __call__(self, code, *args, **kwargs):
        handler_list = self.__handlers.get(code)
        if handler_list is None:
            return
        for handler in handler_list:
            handler(*args, **kwargs)


class CommandQueue(Callback):
    def __init__(self):
        Callback.__init__(self)
        self.__q = multiprocessing.Queue()

    def wait(self, duration):
        """
        Wait for commands and run the command when it receives in the
        given duration.
        @parameter duration
        This method returns after the time of this parameter goes by.
        """
        wakeup_time = time.time() + duration
        while True:
            sleep_time = wakeup_time - time.time()
            if sleep_time <= 0:
                return
            self.__wait(sleep_time)

    def __wait(self, sleep_time):
        try:
            code, args = \
                self.__q.get(block=True, timeout=sleep_time)
            self(code, args)
        except Queue.Empty:
            # If no command is forthcomming within timeout,
            # this path is executed.
            pass

    def push(self, code, args):
        self.__q.put((code, args), block=False)

    def pop_all(self):
        while not self.__q.empty():
            self.__wait(0)


class MonitoringServerInfo:
    def __init__(self, ms_info_dict):
        self.server_id = ms_info_dict["serverId"]
        self.url = ms_info_dict["url"]
        self.type = ms_info_dict["type"]
        self.nick_name = ms_info_dict["nickName"]
        self.user_name = ms_info_dict["userName"]
        self.password = ms_info_dict["password"]
        self.polling_interval_sec = ms_info_dict["pollingIntervalSec"]
        self.retry_interval_sec = ms_info_dict["retryIntervalSec"]
        self.extended_info = ms_info_dict["extendedInfo"]


class ParsedMessage:
    def __init__(self):
        self.error_code = None
        self.message_id = None
        self.message_dict = None
        self.error_message = ""

    def get_error_message(self):
        return "error code: %s, message ID: %s, error message: %s" % \
               (self.error_code, self.message_id, self.error_message)


class ArmInfo:
    def __init__(self):
        self.last_status = str()
        self.failure_reason = str()
        self.last_success_time = str()
        self.last_failure_time = str()
        self.num_success = int()
        self.num_failure = int()


class RabbitMQHapiConnector(RabbitMQConnector):
    def setup(self, transporter_args):
        send_queue_suffix = transporter_args.get("amqp_send_queue_suffix", "-S")
        recv_queue_suffix = transporter_args.get("amqp_recv_queue_suffix", "-T")
        suffix_map = {transporter.DIR_SEND: send_queue_suffix,
                      transporter.DIR_RECV: recv_queue_suffix}
        suffix = suffix_map.get(transporter_args["direction"], "")

        if "amqp_hapi_queue" not in transporter_args:
            transporter_args["amqp_hapi_queue"] = transporter_args["amqp_queue"]
        transporter_args["amqp_queue"] = \
            transporter_args["amqp_hapi_queue"] + suffix
        RabbitMQConnector.setup(self, transporter_args)


class Sender:
    def __init__(self, transporter_args):
        transporter_args["direction"] = transporter.DIR_SEND
        self.__connector = transporter.Factory.create(transporter_args)

    def get_connector(self):
        return self.__connector

    def set_connector(self, connector):
        self.__connector = connector

    def request(self, procedure_name, params, request_id):
        body = {"jsonrpc": "2.0", "method": procedure_name, "params": params}
        if request_id is not None:
            body["id"] = request_id
        self.__connector.call(json.dumps(body))

    def response(self, result, response_id):
        response = json.dumps({"jsonrpc": "2.0", "result": result,
                               "id": response_id})
        self.__connector.reply(response)

    def error(self, error_code, response_id):
        response = json.dumps({"jsonrpc": "2.0",
                               "error": {"code": error_code,
                                         "message": ERROR_DICT[error_code]},
                               "id": response_id})
        self.__connector.reply(response)

    def notify(self, procedure_name, params):
        self.request(procedure_name, params, request_id=None)


class Utils:
    # TODO: We need to specify custom validators
    # TODO: Check the maximum length
    # ToDo Currently, this method does not have notification filter.
    # If we implement notification procedures, should insert notification filter.
    @staticmethod
    def define_transporter_arguments(parser):
        parser.add_argument("--transporter", type=str,
                            default="RabbitMQHapiConnector")
        parser.add_argument("--transporter-module", type=str, default="haplib")

        # TODO: Don't specifiy a sub class of transporter directly.
        #       We'd like to implement the mechanism that automatically
        #       collects transporter's sub classes, loads them,
        #       and calls their define_arguments().
        RabbitMQHapiConnector.define_arguments(parser)

    @staticmethod
    def load_transporter(args):
        (file, pathname, descr) = imp.find_module(args.transporter_module)
        mod = imp.load_module("", file, pathname, descr)
        transporter_class = eval("mod.%s" % args.transporter)
        return transporter_class

    @staticmethod
    def parse_received_message(message, allowed_procedures):
        """
        Parse a received message.
        @return A ParsedMessage object. Each attribute will be set as below.
        - error_code If the parse is succeeded, this attribute is set to None.
                     Othwerwise the error code is set.
        - message_dict A dictionary that corresponds to the received message.
                       If the parse fails, this attribute may be None.
        - message_id A message ID if available. Or None.
        - error_message An error message.
        """

        pm = ParsedMessage()
        pm.error_code, pm.message_dict = \
          Utils._convert_json_to_dict(message)

        # Failed to convert the message to a dictionary
        if pm.error_code is not None:
            return pm

        pm.message_id = pm.message_dict.get("id")

        if pm.message_dict.has_key("error"):
            try:
                Utils._check_error_obj(pm.message_dict)
                pm.error_message = pm.messsage_dict["error"]["message"]
            except KeyError:
                pm.error_message = "Invalid error message: " + message

            return pm

        # The case the message is a reply
        need_id = True
        should_reply = pm.message_dict.has_key("result")

        if not should_reply:
            method = pm.message_dict["method"]
            pm.error_code = Utils.is_allowed_procedure(method,
                                                       allowed_procedures)
            if pm.error_code is not None:
                pm.error_message = "Unsupported method: '%s'" % method
                return pm
            if PROCEDURES_DEFS[method].get("notification"):
                need_id = False

        # 'id' is not included in the message.
        if need_id and pm.message_id is None:
            pm.error_code = ERR_CODE_INVALID_PARAMS
            pm.error_message = "Not found: id"
            return pm

        if should_reply:
            return pm

        pm.error_code, pm.error_message = \
          Utils.validate_arguments(pm.message_dict)
        if pm.error_code is not None:
            return pm

        return pm

    @staticmethod
    def _convert_json_to_dict(json_string):
        try:
            json_dict = json.loads(json_string)
        except ValueError:
            return (ERR_CODE_PARSER_ERROR, None)
        else:
            return (None, json_dict)

    @staticmethod
    def _check_error_obj(error_dict):
        error_dict.has_key["id"]
        error_dict["error"]["code"]
        error_dict["error"]["message"]
        error_dict["error"]["code"]

    @staticmethod
    def is_allowed_procedure(procedure_name, allowed_procedures):
        if procedure_name in allowed_procedures:
            return
        else:
            return ERR_CODE_METHOD_NOT_FOUND

    @staticmethod
    def validate_arguments(json_dict):
        args_dict = PROCEDURES_DEFS[json_dict["method"]]["args"]
        for arg_name, arg_value in args_dict.iteritems():
            try:
                type_expect = type(json_dict["params"][arg_name])
                type_actual = type(arg_value["type"])
                if type_expect != type_actual:
                    msg = "Argument '%s': unexpected type: exp: %s, act: %s" \
                          % (arg_name, type_expect, type_actual)
                    return (ERR_CODE_INVALID_PARAMS, msg)
            except KeyError:
                if arg_value["mandatory"]:
                    msg = "Missing a mandatory paramter: %s" % arg_name
                    return (ERR_CODE_INVALID_PARAMS, msg)
        return (None, None)

    @staticmethod
    def generate_request_id(component_code):
        assert component_code <= 0x7f, \
               "Invalid component code: " + str(component_code)
        req_id = random.randint(1, 0xffffff)
        req_id |= component_code << 24
        return req_id

    @staticmethod
    def translate_unix_time_to_hatohol_time(float_unix_time):
        return datetime.strftime(datetime.fromtimestamp(float_unix_time), "%Y%m%d%H%M%S.%f")

    @staticmethod
    def translate_hatohol_time_to_unix_time(hatohol_time):
        date_time = datetime.strptime(hatohol_time, "%Y%m%d%H%M%S.%f")
        unix_time =  time.mktime(date_time.timetuple())
        return unix_time + date_time.microsecond / float(10 ** 6)

    @staticmethod
    def optimize_server_procedures(valid_procedures_dict, procedures):
        for name in valid_procedures_dict:
            valid_procedures_dict[name] = False

        for procedure in procedures:
            valid_procedures_dict[procedure] = True

    #This method is created on the basis of getting same number of digits under the decimal.
    @staticmethod
    def get_biggest_num_of_dict_array(array, key):
        last_info = None

        digit = int()
        for target_dict in array:
            if isinstance(target_dict[key], int):
                break
            if digit < len(target_dict[key]):
                digit = len(target_dict[key])

        for target_dict in array:
            target_value = target_dict[key]
            if isinstance(target_value, str) or isinstance(target_value, unicode):
                target_value = target_value.zfill(digit)

            if last_info < target_value:
                last_info = target_value

        return last_info

    @staticmethod
    def get_current_hatohol_time():
        utc_now = datetime.utcnow()
        return utc_now.strftime("%Y%m%d%H%M%S.") + str(utc_now.microsecond)
