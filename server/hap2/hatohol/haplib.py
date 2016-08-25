#!/usr/bin/env python
# coding: UTF-8
#
#  Copyright (C) 2015-2016 Project Hatohol
#
#  This file is part of Hatohol.
#
#  Hatohol is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License, version 3
#  as published by the Free Software Foundation.
#
#  Hatohol is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with Hatohol. If not, see
#  <http://www.gnu.org/licenses/>.

"""
This module contains functions and class for creating HAP (Hatohol Arm Plugins)
and provides framework-like components compared to hap.py that includes more
basic ones.
"""

import time
from logging import getLogger
import multiprocessing
import Queue
import json
from datetime import datetime
from datetime import timedelta
import random
import copy
import uuid
from hatohol import hap
from hatohol import hapcommon
from hatohol import transporter
from hatohol.rabbitmqconnector import OverCapacity

logger = getLogger("hatohol.haplib:%s" % hapcommon.get_top_file_name())

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
            "procedures": {"type": list(), "mandatory": True, "max_size": 255},
            "name": {"type": unicode(), "mandatory": True, "max_size": 255},
        }
    },
    "fetchItems": {
        "args": {
            # TODO by 15.09: Validate that size of each hostId is within 255.
            "hostIds": {"type": list(), "mandatory": False},
            "fetchId": {"type": unicode(), "mandatory": True, "max_size": 255},
        }
    },
    "fetchHistory": {
        "args": {
            "hostId": {"type": unicode(), "mandatory": True, "max_size": 255},
            "itemId": {"type": unicode(), "mandatory": True, "max_size": 255},
            # TODO by 15.09: Validate Timestamp.
            "beginTime": {"type": unicode(), "mandatory": True},
            "endTime": {"type": unicode(), "mandatory": True},
            "fetchId": {"type": unicode(), "mandatory": True, "max_size": 255},
        }
    },
    "fetchTriggers": {
        "args": {
            # TODO by 15.09: Validate that size of each hostId is within 255.
            "hostIds": {"type": list(), "mandatory": False},
            "fetchId": {"type": unicode(), "mandatory": True, "max_size": 255}
        }
    },
    "fetchEvents": {
        "args": {
            "lastInfo": {
                "type": unicode(), "mandatory": True, "max_size": 32767
            },
            # TODO by 15.09: Validate the range of the int (max 1000)
            "count": {"type": int(), "mandatory": True},
            "direction": {
                "type": unicode(), "mandatory": True,
                "choices": frozenset(("ASC", "DESC"))
            },
            "fetchId": {"type": unicode(), "mandatory": True, "max_size": 255}
        }
    },
    "updateMonitoringServerInfo": {
        "notification": True,
        "args": {
            "serverId": {"type": int(), "mandatory": True},
            "url": {"type": unicode(), "max_size": 2047, "mandatory": True},
            "type": {"type": unicode(), "max_size": 2047, "mandatory": True},
            "nickName": {"type": unicode(), "max_size": 255, "mandatory": True},
             "userName": {
                "type": unicode(), "max_size": 255, "mandatory": True},
            "password": {"type": unicode(), "max_size": 255, "mandatory": True},
            "pollingIntervalSec": {"type": int(), "mandatory": True},
            "retryIntervalSec": {"type": int(), "mandatory": True},
            "extendedInfo": {
                "type": unicode(), "mandatory": True, "max_size": 32767},
        }
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
    "putItems": {
        "args": {
            # TODO by 15.09: Add a mechanism to validate paramters in
            #                nested dictionaries
            "items": {"type": list(), "mandatory": True},
            "fetchId": {"type": unicode(), "max_size": 255},
        }
    },
    "putHistory": {
        "args": {
            "samples": {"type": list(), "mandatory": True}
        }
    },
    "getLastInfo": {
        # TODO by 15.09: Add a mechanism to validate a direct paramter
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

EVENT_TYPES = frozenset(("GOOD", "BAD", "UNKNOWN", "NOTIFICATION"))
TRIGGER_STATUS = frozenset(("OK", "NG", "UNKNOWN"))
TRIGGER_SEVERITY = frozenset(
    ("UNKNOWN", "INFO", "WARNING", "ERROR", "CRITICAL", "EMERGENCY"))

DEFAULT_MAX_CHUNK_SIZE = 100
MAX_LAST_INFO_SIZE = 32767

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
        self.__q = hap.MultiprocessingQueue()

    def __del__(self):
        self.__q.close()

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
            self.__wait(None)


class MonitoringServerInfo:
    EXTENDED_INFO_RAW  = 0
    EXTENDED_INFO_JSON = 1

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

    def get_extended_info(self, type=EXTENDED_INFO_RAW):
        handlers = {
            self.EXTENDED_INFO_RAW: lambda ext: ext,
            self.EXTENDED_INFO_JSON: lambda ext: json.loads(ext),
        }
        return handlers[type](self.extended_info)

    def __str__(self):
        s = str()
        s += "%s %s %s " % (self.server_id, self.url, self.type)
        s += "%s %s %s " % (self.nick_name, self.user_name, self.password)
        s += "%s %s " % (self.polling_interval_sec, self.retry_interval_sec)
        s += self.extended_info
        return s


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

    def success(self, status="OK"):
        self.last_status = status
        self.last_success_time = hapcommon.get_current_hatohol_time()
        self.num_success += 1

    def fail(self, reason="", status="NG"):
        self.last_status = status
        self.last_failure_time = hapcommon.get_current_hatohol_time()
        self.num_failure += 1
        self.failure_reason = reason

    def get_summary(self):
        msg = "LastStat: %s, NumSuccess: %d (%s), NumFailure: %d (%s): " \
              "FailureReason: %s" % \
                (self.last_status,
                 self.num_success, self.last_success_time,
                 self.num_failure, self.last_failure_time,
                 self.failure_reason)
        return msg


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


"""
Issue HAPI requests and responses.
Some APIs blocks until the response is arrived.
"""
class HapiProcessor:
    def __init__(self, process_id, component_code, sender=None):
        self.__reply_queue = hap.MultiprocessingQueue()
        self.__dispatch_queue = None
        self.__process_id = process_id
        self.__component_code = component_code
        self.__ms_info = None
        self.reset()
        self.__timeout_sec = 30
        self.__sender = sender

    def __del__(self):
        self.__reply_queue.close()

    def set_sender(self, sender):
        self.__sender = sender

    def reset(self):
        self.__previous_hosts = None
        self.__previous_host_groups = None
        self.__previous_host_group_membership = None
        self.__event_last_info = None

    def set_ms_info(self, ms_info):
        self.__ms_info = ms_info

    def get_ms_info(self):
        return self.__ms_info

    def set_dispatch_queue(self, dispatch_queue):
        self.__dispatch_queue = dispatch_queue

    def get_reply_queue(self):
        return self.__reply_queue

    def get_component_code(self):
        return self.__component_code

    def get_sender(self):
        return self.__sender

    def set_timeout_sec(self, timeout_sec):
        if isinstance(timeout_sec, int) and 0 < timeout_sec:
            self.__timeout_sec = timeout_sec
        else:
            logger.error("Inputed value is invalid.")

    def set_event_last_info(self, event_last_info):
        self.__event_last_info = event_last_info

    def __generate_request_id(self, component_code):
        assert component_code <= 0x7f, \
                "Invalid component code: " + str(component_code)
        req_id = random.randint(1, 0xffffff)
        req_id |= component_code << 24
        return req_id

    def get_monitoring_server_info(self):
        """
        Get a MonitoringServerInfo from Hatohol server.
        This method blocks until the response is obtained.
        @return A MonitoringServerInfo object.
        """
        params = ""
        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("getMonitoringServerInfo", params, request_id)
        return MonitoringServerInfo(self.__wait_response(request_id))

    def get_last_info(self, element):
        params = element
        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("getLastInfo", params, request_id)

        return self.__wait_response(request_id)

    def exchange_profile(self, procedures, name="haplib", response_id=None):
        content = {"name": name, "procedures": procedures}
        if response_id is None:
            request_id = self.__generate_request_id(self.__component_code)
            self.__wait_acknowledge(request_id)
            self.__sender.request("exchangeProfile", content, request_id)
            self.__wait_response(request_id)
        else:
            self.__sender.response(content, response_id)

    def put_arm_info(self, arm_info):
        params = {"lastStatus": arm_info.last_status,
                  "failureReason": arm_info.failure_reason,
                  "lastSuccessTime": arm_info.last_success_time,
                  "lastFailureTime": arm_info.last_failure_time,
                  "numSuccess": arm_info.num_success,
                  "numFailure": arm_info.num_failure}

        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("putArmInfo", params, request_id)
        self.__wait_response(request_id)

    def put_hosts(self, hosts, params):
        hosts.sort()
        if self.__previous_hosts == hosts:
            logger.debug("hosts are not changed.")
            return
        params["updateType"] = "ALL"
        params["hosts"] =  hosts
        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("putHosts", params, request_id)
        self.__wait_response(request_id)
        self.__previous_hosts = hosts

    def put_host_groups(self, host_groups, params):
        host_groups.sort()
        if self.__previous_host_groups == host_groups:
            logger.debug("Host groups are not changed.")
            return
        params["updateType"] = "ALL"
        params["hostGroups"] =  host_groups
        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("putHostGroups", params, request_id)
        self.__wait_response(request_id)
        self.__previous_host_groups = host_groups


    def put_host_group_membership(self, hg_membership, params):
        hg_membership.sort()
        if self.__previous_host_group_membership == hg_membership:
            logger.debug("Host group membership is not changed.")
            return

        params["updateType"] = "ALL"
        params["hostGroupMembership"] =  hg_membership
        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("putHostGroupMembership", params, request_id)
        self.__wait_response(request_id)
        self.__previous_host_group_membership = hg_membership

    def put_triggers(self, triggers, params, update_type,
                     last_info=None, fetch_id=None):

        params["updateType"] = update_type
        params["triggers"] = triggers
        if last_info is not None:
            params["lastInfo"] = last_info
        if fetch_id is not None:
            params["fetchId"] = fetch_id

        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("putTriggers", params, request_id)
        self.__wait_response(request_id)

    def get_cached_event_last_info(self):
        if self.__event_last_info is None:
            self.__event_last_info = self.get_last_info("event")
        return self.__event_last_info

    def generate_event_last_info(self, events):
        return hapcommon.get_biggest_num_of_dict_array(events, "eventId")

    def put_events(self, events, params, fetch_id=None,
                   last_info_generator=None):
        if last_info_generator is None:
            last_info_generator = self.generate_event_last_info
        last_info = str(last_info_generator(events))
        params["events"] = events

        if fetch_id is not None:
            params["fetchId"] = fetch_id
            params["mayMoreFlag"] = True
        else:
            params["lastInfo"] = last_info

        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("putEvents", params, request_id)
        self.__wait_response(request_id)

        if fetch_id is None:
            self.set_event_last_info(last_info)

    def put_items(self, items, params, fetch_id):
        params["fetchId"] = fetch_id
        params["items"] = items
        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("putItems", params, request_id)
        self.__wait_response(request_id)

    def put_history(self, samples, params, item_id, fetch_id):
        params["fetchId"] = fetch_id
        params["itemId"] = item_id
        params["samples"] = samples
        request_id = self.__generate_request_id(self.__component_code)
        self.__wait_acknowledge(request_id)
        self.__sender.request("putHistory", params, request_id)
        self.__wait_response(request_id)

    def divide_and_put_data(self, put_func, contents, put_empty_contents=False,
                            *args, **kwargs):
        if put_empty_contents and len(contents) == 0:
            put_func([], dict(), *args, **kwargs)
            return

        chunk_size = DEFAULT_MAX_CHUNK_SIZE
        copy_contents = copy.copy(contents)

        serial_id = 0
        is_last = False
        request_id = str(uuid.uuid4())
        while True:
            num_contents = len(copy_contents)
            if num_contents == 0:
                return
            try:
                count = num_contents / chunk_size
                if num_contents % chunk_size != 0:
                    count += 1

                for num in range(0, count):
                    params = dict()
                    if num == count-1:
                        is_last = True
                    if count > 1:
                        hapcommon.insert_divide_info_to_params(params,
                                                               serial_id,
                                                               is_last,
                                                               request_id)
                    chunk_contents = copy_contents[0: chunk_size]
                    put_func(chunk_contents, params, *args, **kwargs)
                    serial_id += 1
                    del copy_contents[0: chunk_size]
            except OverCapacity:
                chunk_size = chunk_size * 3 / 4
                continue

    def __wait_acknowledge(self, request_id):
        self.__dispatch_queue.put((self.__process_id, request_id))
        self.__dispatch_queue.join()
        try:
            if self.__reply_queue.get(True, self.__timeout_sec) == True:
                pass
            else:
                raise
        except Queue.Empty:
            logger.error("Request(ID: %d) is not accepted." % request_id)
            raise

    def __wait_response(self, request_id):
        try:
            pm = self.__reply_queue.get(True, self.__timeout_sec)
            if pm.message_id != request_id:
                msg = "Got unexpected repsponse. req: " + str(request_id)
                logger.error(msg)
                raise Exception(msg)
            if pm.message_dict["result"] == "FAILURE":
                msg = "Got 'FAILURE' repsponse. req: " + str(request_id)
                logger.error(msg)
                raise Exception(msg)
            return pm.message_dict["result"]

        except ValueError as exception:
            logger.error("Got invalid response.")
            raise
        except Queue.Empty:
            logger.error("Request failed.")
            raise

class ChildProcess:
    def __init__(self):
        self.__process = None

    def get_process(self):
        return self.__process

    def daemonize(self, name):
        self.__process = multiprocessing.Process(target=self, name=name)
        self.__process.daemon = True
        self.__process.start()
        logger.info("deamonized: %s (%s)" % \
                     (self.__process.pid, self.__class__.__name__))

    def terminate(self):
        if self.__process is None:
            return
        self.__process.terminate()
        self.__process.join()
        self.__process = None
        logger.info("terminated: %s", self.__class__.__name__)


class Receiver(ChildProcess):
    def __init__(self, transporter_args, dispatch_queue, procedures):
        ChildProcess.__init__(self)
        transporter_args["direction"] = transporter.DIR_RECV
        self.__connector = transporter.Factory.create(transporter_args)
        self.__dispatch_queue = dispatch_queue
        self.__connector.set_receiver(self.__messenger)
        self.__allowed_procedures = procedures

    def terminate(self):
        ChildProcess.terminate(self)
        self.__connector.close()

    def __is_allowed_procedure(self, procedure_name, allowed_procedures):
        if procedure_name in allowed_procedures:
            return
        else:
            return ERR_CODE_METHOD_NOT_FOUND

    def __validate_arguments(self, method_name, params):
        """
        Validate arguemnts of the received RPC.
        @method_name     A method name to be validated.
        @paramter params A dictionary of the parameters to be validated.
        @return
        A tuple (errorcode, error_message) is returned. if no problem,
        (None, None) is returned.
        """
        args_dict = PROCEDURES_DEFS[method_name]["args"]
        for arg_name, arg_value in args_dict.iteritems():
            try:
                param = params[arg_name]

                # check the paramter type
                type_actual = type(param)
                type_expect = type(arg_value["type"])
                if type_expect != type_actual:
                    msg = "Argument '%s': unexpected type: exp: %s, act: %s" \
                          % (arg_name, type_expect, type_actual)
                    return (ERR_CODE_INVALID_PARAMS, msg)

                # check the paramter's length
                max_len = arg_value.get("max_size")
                if max_len is not None:
                    length = len(param)
                    if length > max_len:
                        msg = "parameter: %s. Over the maixum length %d/%d" \
                              % (arg_name, length, max_len)
                        return (ERR_CODE_INVALID_PARAMS, msg)

                # when the paramter should be one of the choices.
                choices = arg_value.get("choices")
                if choices is not None:
                    if param not in choices:
                        msg = "parameter: %s, value: %s: Not in choices." \
                              % (arg_name, param)
                        return (ERR_CODE_INVALID_PARAMS, msg)

            except KeyError:
                if arg_value["mandatory"]:
                    msg = "Missing a mandatory paramter: %s" % arg_name
                    return (ERR_CODE_INVALID_PARAMS, msg)
        return (None, None)

    def __parse_received_message(self, message, allowed_procedures):
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
        pm.error_code, pm.message_dict = self.__convert_json_to_dict(message)

        # Failed to convert the message to a dictionary
        if pm.error_code is not None:
            return pm

        pm.message_id = pm.message_dict.get("id")

        if pm.message_dict.has_key("error"):
            try:
                self.__check_error_dict(pm.message_dict)
                pm.error_message = pm.message_dict["error"]["message"]
            except KeyError:
                pm.error_message = "Invalid error message: " + message

            return pm

        # The case the message is a reply
        need_id = True
        should_reply = pm.message_dict.has_key("result")

        if not should_reply:
            method = pm.message_dict["method"]
            pm.error_code = self.__is_allowed_procedure(method, allowed_procedures)
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

        params = pm.message_dict["params"]
        pm.error_code, pm.error_message = self.__validate_arguments(method, params)
        if pm.error_code is not None:
            return pm

        return pm

    def __convert_json_to_dict(self, json_string):
        try:
            json_dict = json.loads(json_string)
        except ValueError:
            return (ERR_CODE_PARSER_ERROR, None)
        else:
            return (None, json_dict)

    def __check_error_dict(self, error_dict):
        error_dict["id"]
        error_dict["error"]["code"]
        error_dict["error"]["message"]
        error_dict["error"]["data"]

    def __messenger(self, ch, message):
        parsed = self.__parse_received_message(message, self.__allowed_procedures)
        if parsed.message_id is None and parsed.error_message is not None:
            logger.error(parsed.error_message)
            return
        self.__dispatch_queue.put(("Receiver", parsed))

    def __call__(self):
        # TODO: handle exceptions
        self.__connector.run_receive_loop()


class Dispatcher(ChildProcess):
    def __init__(self, rpc_queue):
        ChildProcess.__init__(self)
        self.__id_res_q_map = {}
        self.__destination_q_map = {}
        self.__dispatch_queue = hap.MultiprocessingJoinableQueue()
        self.__rpc_queue = rpc_queue

    def __del__( self ):
        self.__dispatch_queue.close()

    def attach_destination(self, queue, identifier):
        self.__destination_q_map[identifier] = queue

    def get_dispatch_queue(self):
        return self.__dispatch_queue

    def __acknowledge(self, message):
        wait_id = message[1]
        if wait_id in self.__id_res_q_map:
            logger.error("Ignored duplicated ID: " + str(wait_id))
            return

        try:
            target_queue = self.__destination_q_map[message[0]]
        except KeyError:
            msg = message[0] + " is not registered."
            logger.error(msg)
            return
        self.__id_res_q_map[wait_id] = target_queue
        target_queue.put(True)

    def __is_expected_id_notification(self, contents):
        return isinstance(contents, int)

    def __dispatch(self):
        message = self.__dispatch_queue.get()
        source_process_id, contents = message
        self.__dispatch_queue.task_done()

        if self.__is_expected_id_notification(contents):
            self.__acknowledge(message)
            return

        # Here 'message' must be a payload from the Receiver
        if contents.error_code is not None:
            self.__rpc_queue.put(contents)
            return

        # dispatch the received message to the caller.
        response_id = contents.message_id
        target_queue = self.__id_res_q_map.get(response_id, self.__rpc_queue)

        # RPC shall has 'method'
        if target_queue == self.__rpc_queue:
            if "method" not in contents.message_dict:
                logger.warning("Drop a received message w/o 'method'")
                logger.warning(contents.message_dict)
                return

        target_queue.put(contents)
        if target_queue != self.__rpc_queue:
            del self.__id_res_q_map[response_id]

    def __call__(self):
        while True:
            self.__dispatch()


class BaseMainPlugin(HapiProcessor):
    __COMPONENT_CODE = 0x10
    CB_UPDATE_MONITORING_SERVER_INFO = 1

    def __init__(self, name=None):
        """
        Don't add code that may raise exceptions in __init__.
        They should be in setup().
        """
        HapiProcessor.__init__(self, "Main", self.__COMPONENT_CODE)
        self.__dispatcher = None
        self.__receiver = None
        self.__detect_implemented_procedures()
        self.__callback = Callback()
        if name is not None:
            self.__plugin_name = name
        else:
            self.__plugin_name = self.__class__.__name__
        self.__rpc_queue = None

    def __del__(self):
        HapiProcessor.__del__(self)
        if self.__rpc_queue is not None:
            self.__rpc_queue.close()

    def setup(self, transporter_args):
        """
        This method is bottom half of the constructor and should be called once
        after the instance is created. Raising exceptions is allowed in this
        method.
        """
        self.__sender = Sender(transporter_args)
        HapiProcessor.set_sender(self, self.__sender)
        self.__rpc_queue = hap.MultiprocessingQueue()

        # launch dispatcher process
        self.__dispatcher = Dispatcher(self.__rpc_queue)
        self.__dispatcher.attach_destination(self.get_reply_queue(), "Main")
        dispatch_queue = self.__dispatcher.get_dispatch_queue()
        self.set_dispatch_queue(dispatch_queue)

        # launch receiver process
        self.__receiver = Receiver(transporter_args,
                                   dispatch_queue,
                                   self.__implemented_procedures)

    def destroy(self):
        if self.__receiver is not None:
            self.__receiver.terminate()
            self.__receiver = None

        if self.__dispatcher is not None:
            self.__dispatcher.terminate()
            self.__dispatcher = None

    def register_callback(self, code, arg):
        self.__callback.register(code, arg)

    def __detect_implemented_procedures(self):
        PROCEDURES_MAP = {
            "hap_exchange_profile": "exchangeProfile",
            "hap_fetch_items":      "fetchItems",
            "hap_fetch_history":    "fetchHistory",
            "hap_fetch_triggers":   "fetchTriggers",
            "hap_fetch_events":     "fetchEvents",
            "hap_update_monitoring_server_info": "updateMonitoringServerInfo",
        }
        implement = {}
        for func_name in dir(self):
            procedure_name = PROCEDURES_MAP.get(func_name)
            if procedure_name is None:
                continue
            implement[procedure_name] = eval("self.%s" % func_name)
            logger.info("Detected procedure: %s" % func_name)
        self.__implemented_procedures = implement

    def get_sender(self):
        return self.__sender

    def set_sender(self, sender):
        self.__sender = sender

    def get_dispatcher(self):
        return self.__dispatcher

    def get_plugin_name(self):
        return self.__plugin_name

    def exchange_profile(self, response_id=None):
        HapiProcessor.exchange_profile(
            self, self.__implemented_procedures.keys(),
            response_id=response_id, name=self.__plugin_name)

    def hap_exchange_profile(self, params, request_id):
        self.__optimize_server_procedures(SERVER_PROCEDURES,
                                         params["procedures"])
        self.exchange_profile(response_id=request_id)

    def hap_update_monitoring_server_info(self, params):
        ms_info = MonitoringServerInfo(params)
        self.__callback(self.CB_UPDATE_MONITORING_SERVER_INFO, ms_info)

    def hap_return_error(self, error_code, response_id):
        self.__sender.error(error_code, response_id)

    def request_exit(self):
        """
        Request to exit main loop that is started by __call__().
        """
        self.__rpc_queue.put(None)

    def is_exit_request(self, request):
        return request is None

    def start_receiver(self):
        """
        Launch the process for receiving data from the transporter.
        This method shall be called once before calling __call__().
        """
        self.__receiver.daemonize("Receiver")

    def start_dispatcher(self):
        self.__dispatcher.daemonize("Dispatcher")

    def __optimize_server_procedures(self, valid_procedures_dict, procedures):
        for name in valid_procedures_dict:
            valid_procedures_dict[name] = False

        for procedure in procedures:
            valid_procedures_dict[procedure] = True

    def __call__(self):
        while True:
            msg = self.__rpc_queue.get()
            if self.is_exit_request(msg):
                return

            if msg.error_code is not None:
                self.hap_return_error(msg.error_code, msg.message_id)
                logger.error(msg.get_error_message())
                continue

            request = msg.message_dict
            procedure = self.__implemented_procedures[request["method"]]
            args = [request.get("params")]
            request_id = msg.message_id
            if request_id is not None:
                args.append(request_id)
            procedure(*args)


class BasePoller(HapiProcessor, ChildProcess):
    __COMPONENT_CODE = 0x20
    __CMD_MONITORING_SERVER_INFO = 1
    __DEFAULT_STATUS_LOG_INTERVAL = 600

    ACTIONS = (
        "hosts",
        "host_groups",
        "host_group_membership",
        "triggers",
        "events",
    )

    def __init__(self, *args, **kwargs):
        HapiProcessor.__init__(self, kwargs["process_id"],
                               self.__COMPONENT_CODE, kwargs["sender"])
        ChildProcess.__init__(self)

        self.__pollingInterval = 30
        self.__retryInterval = 10
        self.__command_queue = CommandQueue()
        self.__command_queue.register(self.__CMD_MONITORING_SERVER_INFO,
                                      self.__set_ms_info)

        self.__poll_actions = []
        polling_targets_list = kwargs.get("polling_targets")
        if polling_targets_list is None:
            polling_targets_list = self.ACTIONS
        polling_targets = set(polling_targets_list)
        for action in self.ACTIONS:
            if action in polling_targets:
                func_name = "self.poll_%s" % action
                self.__poll_actions.append(eval(func_name))
                logger.info("Enable polling: %s" % action)
                polling_targets.remove(action)

        for unknown_action in polling_targets:
            logger.warning("Unknown polling action: %s" % unknown_action)

        # The first polling result should be logged
        self.__next_log_status_time = datetime.now()
        interval = kwargs.get("status_log_interval",
                              self.__DEFAULT_STATUS_LOG_INTERVAL)
        logger.info("Minimum status logging interval: %d" % interval)
        self.__log_status_interval = timedelta(seconds=interval)

    def poll(self):
        ctx = self.poll_setup()
        for action in self.__poll_actions:
            action()

    def get_command_queue(self):
        return self.__command_queue

    def poll_setup(self):
        return None

    def poll_hosts(self):
        pass

    def poll_host_groups(self):
        pass

    def poll_host_group_membership(self):
        pass

    def poll_triggers(self):
        pass

    def poll_events(self):
        pass

    def on_aborted_poll(self):
        pass

    def log_status(self, arm_info):
        """
        Log the status periodically to see if the plugin is woring well.
        @param arm_info ArmInfo object that has the latest status.
        """
        now = datetime.now()
        if now >= self.__next_log_status_time:
            logger.info(arm_info.get_summary())
            self.__next_log_status_time = now + self.__log_status_interval

    def set_ms_info(self, ms_info):
        self.__command_queue.push(self.__CMD_MONITORING_SERVER_INFO, ms_info)

    def __set_ms_info(self, ms_info):
        HapiProcessor.set_ms_info(self, ms_info)
        self.__pollingInterval = ms_info.polling_interval_sec
        self.__retryInterval = ms_info.retry_interval_sec
        logger.info("Polling inverval: %d/%d",
                     self.__pollingInterval, self.__retryInterval)
        raise hap.Signal(restart=True)

    def __call__(self):
        arm_info = ArmInfo()
        while (True):
            self.__poll_in_try_block(arm_info)

    def __poll_in_try_block(self, arm_info):
        succeeded = False
        failure_reason = ""
        try:
            self.__command_queue.pop_all()
            self.poll()
            succeeded = True
        except:
            exctype, value = hap.handle_exception()
            if exctype is hap.Signal:
                if value.restart:
                    return
            else:
                failure_reason = "%s, %s" % (exctype, value)

        if succeeded:
            sleep_time = self.__pollingInterval
            arm_info.success()
        else:
            sleep_time = self.__retryInterval
            self.on_aborted_poll()
            arm_info.fail()

        # Send ArmInfo
        try:
            arm_info.failure_reason = failure_reason
            self.put_arm_info(arm_info)
            self.log_status(arm_info)
        except:
            hap.handle_exception()

        try:
            self.__command_queue.wait(sleep_time)
        except:
            hap.handle_exception()
