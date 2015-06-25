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
import unittest
import haplib
import time
import re
import common
import transporter
import os
import json
from collections import namedtuple
import argparse

class Gadget:
    def __init__(self):
        self.num_called = 0

    def __call__(self, arg1, arg2=None, arg3=None, arg4=None):
        self.args = (arg1, arg2, arg3, arg4)
        self.num_called += 1


class TestHaplib_handle_exception(unittest.TestCase):
    def test_handle_exception(self):
        obj = Gadget()
        try:
            raise obj
        except:
            exctype, value = haplib.handle_exception()
        self.assertEquals(Gadget, exctype)
        self.assertEquals(obj, value)

    def test_handle_exception_on_raises(self):
        try:
            raise TypeError
        except:
            self.assertRaises(TypeError, haplib.handle_exception, (TypeError,))


class TestHaplib_Signal(unittest.TestCase):
    def test_default(self):
        obj = haplib.Signal()
        self.assertEquals(False, obj.restart)

    def test_restart_is_true(self):
        obj = haplib.Signal(restart=True)
        self.assertEquals(True, obj.restart)


class TestHaplib_Callback(unittest.TestCase):
    def test_register_and_call(self):
        cb = haplib.Callback()
        handler = Gadget()
        cb.register(1, handler)
        arg1 = "a"
        arg2 = None
        arg3 = 1.3
        arg4 = True
        command_code = 1
        cb(command_code, arg1, arg2, arg3=arg3, arg4=arg4)
        self.assertEquals((arg1, arg2, arg3, arg4), handler.args)

    def test_call_with_no_handlers(self):
        cb = haplib.Callback()
        command_code = 1
        cb(command_code)


class CommandQueue(unittest.TestCase):
    def test_push_and_wait(self):
        code = 2
        args = ('a', 2, -1.5)
        cq = haplib.CommandQueue()
        gadz = Gadget()
        cq.register(code, gadz)
        cq.push(code, args)
        # This is not smart.
        #The processing won't be completed within 'duration'
        duration = 0.1
        cq.wait(duration)
        self.assertEquals((args, None, None, None), gadz.args)

    def test_pop_all(self):
        code = 3
        num_push = 5
        args = ('a', 2, -1.5)
        cq = haplib.CommandQueue()
        gadz = Gadget()
        cq.register(code, gadz)
        self.assertEquals(0, gadz.num_called)
        [cq.push(code, args) for i in range(0, num_push)]
        # Too ugly. But we don't have other way to wait for completion of
        # processing in the background thread.
        while cq.__q.empty():
            time.sleep(0.01)
        cq.pop_all()
        self.assertEquals(num_push, gadz.num_called)


class MonitoringServerInfo(unittest.TestCase):
    def test_create(self):
        info_dict = {
            "serverId": 10,
            "url": "http://who@where:foo/hoge",
            "type": "8e632c14-d1f7-11e4-8350-d43d7e3146fb",
            "nickName": "carrot",
            "userName": "ninjin",
            "password": "radish",
            "pollingIntervalSec": 30,
            "retryIntervalSec": 15,
            "extendedInfo": "Time goes by."
        }

        key_map = {}
        for key in info_dict.keys():
            snake = re.sub("([A-Z])", lambda x: "_" + x.group(1).lower(), key)
            key_map[key] = snake

        ms_info = haplib.MonitoringServerInfo(info_dict)
        for key, val in info_dict.items():
            actual = eval("ms_info.%s" % key_map[key])
            self.assertEquals(actual, val)


class ParsedMessage(unittest.TestCase):
    def test_create(self):
        pm = haplib.ParsedMessage()
        self.assertIsNone(pm.error_code)
        self.assertIsNone(pm.message_id)
        self.assertIsNone(pm.message_dict)
        self.assertEquals("", pm.error_message)

    def test_get_error_message(self):
        pm = haplib.ParsedMessage()
        actual = "error code: None, message ID: None, error message: "
        self.assertEquals(actual, pm.get_error_message())


class ArmInfo(unittest.TestCase):
    def test_create(self):
        arm_info = haplib.ArmInfo()
        self.assertEquals("", arm_info.last_status)
        self.assertEquals("", arm_info.failure_reason)
        self.assertEquals("", arm_info.last_success_time)
        self.assertEquals("", arm_info.last_failure_time)
        self.assertEquals(0, arm_info.num_success)
        self.assertEquals(0, arm_info.num_failure)


class Sender(unittest.TestCase):
    def test_get_connector(self):
        transporter_args = {"class": transporter.Transporter}
        test_sender = haplib.Sender(transporter_args)
        result_connector = test_sender.get_connector()
        self.assertEquals(test_sender._Sender__connector, result_connector)

    def test_set_connector(self):
        transporter_args = {"class": transporter.Transporter}
        test_sender = haplib.Sender(transporter_args)
        test_sender.set_connector("test")
        self.assertEquals("test", test_sender._Sender__connector)

    def test_request(self):
        transporter_args = {"class": transporter.Transporter}
        test_sender = haplib.Sender(transporter_args)
        common.assertNotRaises(test_sender.request,
                               "test_procedure_name", "test_param", 1)

    def test_response(self):
        transporter_args = {"class": transporter.Transporter}
        test_sender = haplib.Sender(transporter_args)
        common.assertNotRaises(test_sender.response,
                               "test_result", 1)

    def test_error(self):
        transporter_args = {"class": transporter.Transporter}
        test_sender = haplib.Sender(transporter_args)
        common.assertNotRaises(test_sender.error, -32700, 1)

    def test_notifiy(self):
        transporter_args = {"class": transporter.Transporter}
        test_sender = haplib.Sender(transporter_args)
        common.assertNotRaises(test_sender.notify, "test_notify", 1)


class Utils(unittest.TestCase):
    def test_define_transporter_arguments(self):
        test_parser = argparse.ArgumentParser()
        common.assertNotRaises(haplib.Utils.define_transporter_arguments,
                               test_parser)

    def test_load_transporter(self):
        test_transport_arguments = namedtuple("transport_argument",
                                              "transporter_module transporter")
        test_transport_arguments.transporter_module = "haplib"
        test_transport_arguments.transporter = "RabbitMQHapiConnector"

        common.assertNotRaises(haplib.Utils.load_transporter,
                               test_transport_arguments)

    def test_parse_received_message_invalid_json(self):
        test_message = "invalid_message"
        result = haplib.Utils.parse_received_message(test_message, None)

        self.assertEquals(haplib.ERR_CODE_PARSER_ERROR, result.error_code)

    def test_parse_received_message_not_implement(self):
        test_message = '{"method": "test_procedure", "params": "test_params", "id": 1, "jsonrpc": "2.0"}'
        result = haplib.Utils.parse_received_message(test_message,
                                                     ["exchangeProfile"])

        self.assertEquals(haplib.ERR_CODE_METHOD_NOT_FOUND, result.error_code)

    def test_parse_received_message_invalid_argument(self):
        test_message = '{"method": "exchangeProfile", "params": {"name":1, "procedures":"test"}, "id": 1, "jsonrpc": "2.0"}'
        result = haplib.Utils.parse_received_message(test_message,
                                                     ["exchangeProfile"])

        self.assertEquals(haplib.ERR_CODE_INVALID_PARAMS, result.error_code)

    def test_parse_received_message_valid_json(self):
        test_message = '{"method": "exchangeProfile", "params": {"name":"test_name", "procedures":["exchangeProfile"]}, "id": 1, "jsonrpc": "2.0"}'
        result = haplib.Utils.parse_received_message(test_message,
                                                     ["exchangeProfile"])

        exact_dict = json.loads(test_message)
        self.assertEquals(exact_dict, result.message_dict)

    def test_parse_received_message_response(self):
        test_message = '{"result":"test_result","id":1,"jsonrpc": "2.0"}'
        result = haplib.Utils.parse_received_message(test_message,
                                                     ["exchangeProfile"])

        exact_dict = json.loads(test_message)
        self.assertEquals(exact_dict, result.message_dict)

    def test_parse_received_message_valid_error_reply(self):
        test_message = '{"error": {"code": 1, "message": "test_message", "data": "test_data"},"id": 1}'
        result = haplib.Utils.parse_received_message(test_message, None)
        self.assertEquals("test_message", str(result.error_message))
        self.assertEquals(1, result.message_id)

    def test_parse_received_message_invalid_error_reply(self):
        test_message = '{"error": {"code": 1, "message": "test_message"},"id": 1}'
        result = haplib.Utils.parse_received_message(test_message, None)
        self.assertEquals("Invalid error message: " + test_message,
                          str(result.error_message))
        self.assertEquals(1, result.message_id)

    def test_convert_json_to_dict_success(self):
        test_json_string = '{"test_key":"test_value"}'

        exact_result = json.loads(test_json_string)
        unnesessary_result, result = \
                haplib.Utils._convert_json_to_dict(test_json_string)
        self.assertEquals(result, exact_result)

    def test_convert_json_to_dict_failure(self):
        test_json_string = '{"test_key": test_value}'

        exact_result = (-32700, None)
        result = haplib.Utils._convert_json_to_dict(test_json_string)
        self.assertEquals(exact_result, result)

    def test_check_error_dict_success(self):
        error_dict = {"error": {"code": 1, "message": "test_message", "data": "test_data"},"id": 1}
        common.assertNotRaises(haplib.Utils._check_error_dict, error_dict)

    def test_check_error_dict_failure(self):
        error_dict = {"error": {"code": 1, "message": "test_message"},"id": 1}
        try:
            haplib.Utils._check_error_dict(error_dict)
            raise
        except:
            pass

    def test_is_allowed_procedure_success(self):
        result = haplib.Utils.is_allowed_procedure("exchangeProfile",
                                                   ["exchangeProfile"])
        self.assertIsNone(result)

    def test_is_allowed_procedure_failure(self):
        result = haplib.Utils.is_allowed_procedure("test_procedure_name",
                                                   ["exchangeProfile"])
        self.assertEquals(result, -32601)

    def test_validate_arguments_success(self):
        test_json_string = '{"method": "exchangeProfile", "params": {"name":"test_name", "procedures":["exchangeProfile"]}}'
        test_json_dict = json.loads(test_json_string)
        result = haplib.Utils.validate_arguments(test_json_dict)
        self.assertTrue((None, None), result)

    def test_validate_arguments_fail(self):
        test_json_string = '{"method": "exchangeProfile", "params": {"name":"test_name", "procedures":"exchangeProfile"}}'
        test_json_dict = json.loads(test_json_string)
        result = haplib.Utils.validate_arguments(test_json_dict)
        self.assertEquals(result[0], -32602)

    def test_generate_request_id(self):
        result = haplib.Utils.generate_request_id(0x01)

        self.assertTrue(0 <= result <= 1111111111111)

    def test_translate_unix_time_to_hatohol_time(self):
        result = haplib.Utils.translate_unix_time_to_hatohol_time(0)
        self.assertEquals(result, "19700101000000")
        result = haplib.Utils.translate_unix_time_to_hatohol_time(0.123456)
        self.assertEquals(result, "19700101000000.123456")

    def test_translate_hatohol_time_to_unix_time(self):
        result = haplib.Utils.translate_hatohol_time_to_unix_time("19700101000000.123456789")
        # This result is utc time
        self.assertAlmostEquals(result, 0.123456789, delta=0.000000001)

    def test_optimize_server_procedures(self):
        test_procedures_dict = {"exchangeProfile": True,
                                "updateTriggers": True}
        test_procedures = ["exchangeProfile"]

        haplib.Utils.optimize_server_procedures(test_procedures_dict,
                                                test_procedures)
        self.assertTrue(test_procedures_dict["exchangeProfile"])
        self.assertFalse(test_procedures_dict["updateTriggers"])

    def test_get_biggest_num_of_dict_array(self):
        test_target_array = [{"test_value": 3},
                             {"test_value": 7},
                             {"test_value": 9}]
        result = haplib.Utils.get_biggest_num_of_dict_array(test_target_array,
                                                            "test_value")
        self.assertEquals(result, 9)

    def test_get_current_hatohol_time(self):
        result = haplib.Utils.get_current_hatohol_time()

        self.assertTrue(14 < len(result) < 22)
        ns = int(result[15: 21])
        self.assertTrue(0 <= ns < 1000000)
