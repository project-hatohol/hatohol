#!/usr/bin/env python
"""
  Copyright (C) 2016 Project Hatohol

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
import time
import re
import testutils
import transporter
import json
import Queue
from hatohol import hap
from hatohol import haplib
from hatohol import hapcommon

class Gadget:
    def __init__(self):
        self.num_called = 0

    def __call__(self, arg1, arg2=None, arg3=None, arg4=None):
        self.args = (arg1, arg2, arg3, arg4)
        self.num_called += 1


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
    INFO_DICT = {
        "serverId": 10,
        "url": "http://who@where:foo/hoge",
        "type": "8e632c14-d1f7-11e4-8350-d43d7e3146fb",
        "nickName": "carrot",
        "userName": "ninjin",
        "password": "radish",
        "pollingIntervalSec": 30,
        "retryIntervalSec": 15,
        "extendedInfo": '{"title": "Time goes by.", "number": 101}'
    }

    def test_create(self):
        key_map = {}
        for key in self.INFO_DICT.keys():
            snake = re.sub("([A-Z])", lambda x: "_" + x.group(1).lower(), key)
            key_map[key] = snake

        ms_info = haplib.MonitoringServerInfo(self.INFO_DICT)
        for key, val in self.INFO_DICT.items():
            actual = eval("ms_info.%s" % key_map[key])
            self.assertEquals(actual, val)

    def test_get_extended_info_raw(self):
        type = haplib.MonitoringServerInfo.EXTENDED_INFO_RAW
        expect = self.INFO_DICT["extendedInfo"]
        self.__assert_get_extended_info(type, expect)

    def test_get_extended_info_json(self):
        type = haplib.MonitoringServerInfo.EXTENDED_INFO_JSON
        expect = {"title": "Time goes by.", "number": 101}
        self.__assert_get_extended_info(type, expect)

    def __assert_get_extended_info(self, type, expect):
        ms_info = haplib.MonitoringServerInfo(self.INFO_DICT)
        ext = ms_info.get_extended_info(type)
        self.assertEquals(ext, expect)


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

    def test_success(self):
        arm_info = haplib.ArmInfo()
        time0 = float(hapcommon.get_current_hatohol_time())
        arm_info.success()
        time1 = float(hapcommon.get_current_hatohol_time())
        self.assertEquals(arm_info.last_status, "OK")
        self.assertEquals(arm_info.num_success, 1)
        arm_info_time = float(arm_info.last_success_time)
        self.assertGreaterEqual(arm_info_time, time0)
        self.assertGreaterEqual(time1, arm_info_time)

    def test_fail(self):
        arm_info = haplib.ArmInfo()
        time0 = float(hapcommon.get_current_hatohol_time())
        arm_info.fail("Reason")
        time1 = float(hapcommon.get_current_hatohol_time())
        self.assertEquals(arm_info.last_status, "NG")
        self.assertEquals(arm_info.failure_reason, "Reason")
        self.assertEquals(arm_info.num_failure, 1)
        arm_info_time = float(arm_info.last_failure_time)
        self.assertGreaterEqual(arm_info_time, time0)
        self.assertGreaterEqual(time1, arm_info_time)

    def test_get_summary(self):
        arm_info = haplib.ArmInfo()
        arm_info.last_status = "OK"
        arm_info.failure_reason = "ABC DEF"
        arm_info.last_success_time = "20150709141523.123"
        arm_info.last_failure_time = "20150701235500.789"
        arm_info.num_success = 512
        arm_info.num_failure = 1024

        expect =  "LastStat: OK, NumSuccess: 512 (20150709141523.123), " \
                  "NumFailure: 1024 (20150701235500.789): " \
                  "FailureReason: ABC DEF"
        self.assertEquals(arm_info.get_summary(), expect)


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
        testutils.assertNotRaises(test_sender.request,
                               "test_procedure_name", "test_param", 1)

    def test_response(self):
        transporter_args = {"class": transporter.Transporter}
        test_sender = haplib.Sender(transporter_args)
        testutils.assertNotRaises(test_sender.response,
                               "test_result", 1)

    def test_error(self):
        transporter_args = {"class": transporter.Transporter}
        test_sender = haplib.Sender(transporter_args)
        testutils.assertNotRaises(test_sender.error, -32700, 1)

    def test_notifiy(self):
        transporter_args = {"class": transporter.Transporter}
        test_sender = haplib.Sender(transporter_args)
        testutils.assertNotRaises(test_sender.notify, "test_notify", 1)


class HapiProcessor(unittest.TestCase):
    def __create_test_instance(self, connector_class=None):
        sender = haplib.Sender({"class": transporter.Transporter})
        obj = haplib.HapiProcessor("test", 0x01, sender)
        obj.set_dispatch_queue(DummyQueue())
        if not connector_class:
            return obj
        connector = connector_class(obj.get_reply_queue())
        sender.set_connector(connector)
        return obj, connector

    def test_reset(self):
        targets = ("__previous_hosts", "__previous_host_groups",
                   "__previous_host_group_membership", "__event_last_info")

        # Set arbitary data to each member
        hapiproc = self.__create_test_instance()
        for attr_name in targets:
            testutils.set_priv_attr(hapiproc, attr_name, "Test Data")

        hapiproc.reset()
        for attr_name in targets:
            self.assertIsNone(testutils.get_priv_attr(hapiproc, attr_name))

    def test_set_ms_info(self):
        hapiproc = self.__create_test_instance()
        test_ms = "test_ms"
        hapiproc.set_ms_info(test_ms)
        self.assertEquals(test_ms,
                          testutils.get_priv_attr(hapiproc, "__ms_info"))

    def test_get_ms_info(self):
        hapiproc = self.__create_test_instance()
        test_ms = "test_ms"
        testutils.set_priv_attr(hapiproc, "__ms_info", test_ms)
        self.assertEquals(hapiproc.get_ms_info(), test_ms)

    def test_set_dispatch_queue(self):
        hapiproc = self.__create_test_instance()
        test_dispatch_queue = DummyQueue()
        hapiproc.set_dispatch_queue(test_dispatch_queue)
        self.assertEquals(
            testutils.get_priv_attr(hapiproc, "__dispatch_queue"),
            test_dispatch_queue)

    def test_get_component_code(self):
        hapiproc = self.__create_test_instance()
        self.assertEquals(
            hapiproc.get_component_code(),
            testutils.get_priv_attr(hapiproc, "__component_code"))

    def test_get_reply_queue(self):
        hapiproc = self.__create_test_instance()
        self.assertEquals(
            hapiproc.get_reply_queue(),
            testutils.get_priv_attr(hapiproc, "__reply_queue"))

    def test_get_sender(self):
        hapiproc = self.__create_test_instance()
        self.assertEquals(hapiproc.get_sender(),
                          testutils.get_priv_attr(hapiproc, "__sender"))

    def test_get_monitoring_server_info(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        connector.enable_ms_flag()
        self.assertIsInstance(hapiproc.get_monitoring_server_info(),
                              haplib.MonitoringServerInfo)

    def test_get_last_info(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        self.assertEquals(hapiproc.get_last_info("test_element"), "SUCCESS")

    def test_exchange_profile_request(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        self.assertIsNone(hapiproc.exchange_profile("test_params"))

    def test_exchange_profile_response(self):
        hapiproc = self.__create_test_instance()
        self.assertIsNone(
            hapiproc.exchange_profile("test_params", response_id=1))

    def test_put_arm_info(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        testutils.assertNotRaises(hapiproc.put_arm_info, haplib.ArmInfo())

    def test_put_hosts(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        hapiproc.put_hosts(["test_host"])

    def test_put_host_groups(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        hapiproc.put_host_groups(["test_host_group"])

    def test_put_host_group_membership(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        hapiproc.put_host_group_membership(["test_host_group_membership"])

    def test_put_triggers(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        hapiproc.put_triggers(["test_triggers"], "ALL")

    def test_get_cached_event_last_info(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        self.assertEquals(hapiproc.get_cached_event_last_info(), "SUCCESS")

    def test_put_events(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        hapiproc.put_events([{"eventId": 123, "test_events":"test"}])

    def test_put_items(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        fetch_id = 543
        items = [{"itemId": "123", "host_id": "FOOOOOO"}]
        hapiproc.put_items(items, fetch_id)
        # TODO: Check if fetch_id and items shall be passed to the lower layer

    def test_put_history(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        fetch_id = 543
        item_id = 111
        samples = [{"value": "123", "time": "20150321151321"}]
        hapiproc.put_history(samples, item_id, fetch_id)
        # TODO: Check if fetch_id and items shall be passed to the lower layer

    def test_wait_acknowledge(self):
        hapiproc, connector = self.__create_test_instance(ConnectorForTest)
        hapiproc.get_reply_queue().put(True)
        wait_acknowledge = testutils.get_priv_attr(hapiproc,
                                                   "__wait_acknowledge")
        wait_acknowledge(1)

    def test_wait_acknowledge_timeout(self):
        hapiproc = self.__create_test_instance()
        hapiproc.set_timeout_sec(1)
        wait_acknowledge = testutils.get_priv_attr(hapiproc,
                                                   "__wait_acknowledge")
        self.assertRaises(Queue.Empty, wait_acknowledge, 1)

    def test_wait_response(self):
        hapiproc = self.__create_test_instance()
        test_result = "test_result"
        test_id = 1
        pm = haplib.ParsedMessage()
        pm.message_dict = {"id": test_id, "result": test_result}
        pm.message_id = test_id
        hapiproc.get_reply_queue().put(pm)
        wait_response = testutils.get_priv_attr(hapiproc, "__wait_response")
        self.assertEquals(wait_response(test_id), test_result)

    def test_wait_response_timeout(self):
        hapiproc = self.__create_test_instance()
        hapiproc.set_timeout_sec(1)
        test_id = 1
        wait_response = testutils.get_priv_attr(hapiproc, "__wait_response")
        self.assertRaises(Queue.Empty, wait_response, test_id)

    def test_generate_request_id(self):
        hapiproc = self.__create_test_instance()
        result = hapiproc.__generate_request_id(0x01)

        self.assertTrue(0 <= result <= 1111111111111)


class ChildProcess(unittest.TestCase):

    class TestChild(haplib.ChildProcess):
        def __call__(self):
            pass

    def test_constructor(self):
        testutils.assertNotRaises(haplib.ChildProcess)

    def test_get_process_before_deamonize(self):
        cp = haplib.ChildProcess()
        self.assertEquals(cp.get_process(), None)

    def test_daemonize(self):
        cp = ChildProcess.TestChild()
        cp.daemonize()
        self.assertNotEquals(cp.get_process(), None)

    def test_terminate_before_daemonize(self):
        cp = ChildProcess.TestChild()
        cp.terminate()

    def test_terminate(self):
        cp = ChildProcess.TestChild()
        cp.daemonize()
        cp.terminate()


class Receiver(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        transporter_args = {"class": transporter.Transporter}
        cls.__test_queue = DummyQueue()
        cls.receiver = haplib.Receiver(transporter_args, cls.__test_queue, None)

    def test_messenger(self):
        messenger = testutils.get_priv_attr(self.receiver, "__messenger")
        testutils.assertNotRaises(messenger, None, "test_message")

    def test_messenger_get_broken_error(self):
        messenger = testutils.get_priv_attr(self.receiver, "__messenger")
        testutils.assertNotRaises(messenger, None, '{"error": "test"}')

    def test_call(self):
        testutils.assertNotRaises(self.receiver.__call__)

    def test_daemonize(self):
        testutils.assertNotRaises(self.receiver.daemonize)

    def test_parse_received_message_invalid_json(self):
        test_message = "invalid_message"
        result = self.receiver.__parse_received_message(test_message, None)

        self.assertEquals(haplib.ERR_CODE_PARSER_ERROR, result.error_code)

    def test_parse_received_message_not_implement(self):
        test_message = '{"method": "test_procedure", "params": "test_params", "id": 1, "jsonrpc": "2.0"}'
        result = self.receiver.__parse_received_message(test_message,
                                                        ["exchangeProfile"])

        self.assertEquals(haplib.ERR_CODE_METHOD_NOT_FOUND, result.error_code)

    def test_parse_received_message_invalid_argument(self):
        test_message = '{"method": "exchangeProfile", "params": {"name":1, "procedures":"test"}, "id": 1, "jsonrpc": "2.0"}'
        result = self.receiver.__parse_received_message(test_message,
                                                        ["exchangeProfile"])

        self.assertEquals(haplib.ERR_CODE_INVALID_PARAMS, result.error_code)

    def test_parse_received_message_valid_json(self):
        test_message = '{"method": "exchangeProfile", "params": {"name":"test_name", "procedures":["exchangeProfile"]}, "id": 1, "jsonrpc": "2.0"}'
        result = self.receiver.__parse_received_message(test_message,
                                                        ["exchangeProfile"])

        exact_dict = json.loads(test_message)
        self.assertEquals(exact_dict, result.message_dict)

    def test_parse_received_message_response(self):
        test_message = '{"result":"test_result","id":1,"jsonrpc": "2.0"}'
        result = self.receiver.__parse_received_message(test_message,
                                                        ["exchangeProfile"])

        exact_dict = json.loads(test_message)
        self.assertEquals(exact_dict, result.message_dict)

    def test_parse_received_message_valid_error_reply(self):
        test_message = '{"error": {"code": 1, "message": "test_message", "data": "test_data"},"id": 1}'
        result = self.receiver.__parse_received_message(test_message, None)
        self.assertEquals("test_message", str(result.error_message))
        self.assertEquals(1, result.message_id)

    def test_parse_received_message_invalid_error_reply(self):
        test_message = '{"error": {"code": 1, "message": "test_message"},"id": 1}'
        result = self.receiver.__parse_received_message(test_message, None)
        self.assertEquals("Invalid error message: " + test_message,
                          str(result.error_message))
        self.assertEquals(1, result.message_id)

    def test_convert_json_to_dict_success(self):
        test_json_string = '{"test_key":"test_value"}'

        exact_result = json.loads(test_json_string)
        convert_json_to_dict = self.receiver.__convert_json_to_dict
        unnesessary_result, result = convert_json_to_dict(test_json_string)
        self.assertEquals(result, exact_result)

    def test_convert_json_to_dict_failure(self):
        test_json_string = '{"test_key": test_value}'

        exact_result = (-32700, None)
        convert_json_to_dict = self.receiver.__convert_json_to_dict
        result = convert_json_to_dict(test_json_string)
        self.assertEquals(exact_result, result)

    def test_check_error_dict_success(self):
        error_dict = {"error": {"code": 1, "message": "test_message", "data": "test_data"},"id": 1}
        check_error_dict = self.receiver.__check_error_dict
        testutils.assertNotRaises(check_error_dict, error_dict)

    def test_check_error_dict_failure(self):
        error_dict = {"error": {"code": 1, "message": "test_message"},"id": 1}
        try:
            hapcommon.__check_error_dict(error_dict)
            raise
        except:
            pass

    def test_is_allowed_procedure_success(self):
        result = self.receiver.__is_allowed_procedure("exchangeProfile",
                                                      ["exchangeProfile"])
        self.assertIsNone(result)

    def test_is_allowed_procedure_failure(self):
        result = self.receiver.__is_allowed_procedure("test_procedure_name",
                                                      ["exchangeProfile"])
        self.assertEquals(result, -32601)

    def test_validate_arguments_success(self):
        test_params_str = '{"params": {"name":"test_name", "procedures":["exchangeProfile"]}}'
        result = self.receiver.__validate_arguments("exchangeProfile",
                                                    json.loads(test_params_str))
        self.assertTrue((None, None), result)

    def test_validate_arguments_fail(self):
        test_params_str = '{"name":"test_name", "procedures":"exchangeProfile"}'
        result = self.receiver.__validate_arguments("exchangeProfile",
                                                    json.loads(test_params_str))
        self.assertEquals(result[0], -32602)

    def __assert_validate_arguments_for_max_len(self, length, expect_code):
        params = {"procedures": []}
        params["name"] = reduce(lambda x,y: x+y, [u"A" for i in range(length)])
        self.assertEquals(len(params["name"]), length)
        result = self.receiver.__validate_arguments("exchangeProfile", params)
        self.assertEquals(result[0], expect_code)

    def test_validate_arguments_with_max_len(self):
        self.__assert_validate_arguments_for_max_len(255, None)

    def test_validate_arguments_with_max_len_plus_1(self):
        # We expect INVALID PARAMS
        self.__assert_validate_arguments_for_max_len(256, -32602)

    def __assert_validate_arguments_with_choices(self, direction, expect_code):
        test_params_str = \
            '{"lastInfo":"12345", "count": 100, ' + \
            '"direction": "%s", "fetchId": "6789"}' % direction
        result = self.receiver.__validate_arguments("fetchEvents",
                                                    json.loads(test_params_str))
        self.assertEquals(result[0], expect_code)

    def test_validate_arguments_with_choices(self):
        self.__assert_validate_arguments_with_choices("ASC", None)
        self.__assert_validate_arguments_with_choices("DESC", None)

    def test_validate_arguments_with_out_of_choices(self):
        self.__assert_validate_arguments_with_choices("Fire", -32602)


class Dispatcher(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rpc_queue = DummyQueue()
        cls.__dispatcher = haplib.Dispatcher(rpc_queue)

    def test_attach_destination(self):
        self.__dispatcher.attach_destination("test_queue", "test")
        destination_q_map = testutils.get_priv_attr(self.__dispatcher,
                                                    "__destination_q_map")
        self.assertEquals("test_queue", destination_q_map["test"])

    def test_get_dispatch_queue(self):
        dispatch_queue = testutils.get_priv_attr(self.__dispatcher,
                                                 "__dispatch_queue")
        self.assertEquals(self.__dispatcher.get_dispatch_queue(), dispatch_queue)

    def test_acknowledge(self):
        destination_queue = DummyQueue()
        test_id = "test"
        self.__dispatcher.attach_destination(destination_queue, test_id)
        acknowledge = testutils.get_priv_attr(self.__dispatcher,
                                              "__acknowledge")
        test_message = (test_id, 1)
        testutils.assertNotRaises(acknowledge, test_message)

    def is_expented_id_notification(self):
        is_expected_id_notification = \
            testutils.get_priv_attr(self.__dispatcher,
                                    "__is_expenced_id_notification")
        test_contents = 1
        self.assertTrue(is_expected_id_notification(test_contents))

    def test_dispatch_receive_id_notification(self):
        destination_queue = DummyQueue()
        test_id = "test"
        test_contents = 1
        test_message = (test_id, test_contents)
        dispatch_queue = testutils.get_priv_attr(self.__dispatcher,
                                                 "__dispatch_queue")
        dispatch_queue.put(test_message)
        self.__dispatcher.attach_destination(destination_queue, test_id)
        dispatch = testutils.get_priv_attr(self.__dispatcher, "__dispatch")
        testutils.assertNotRaises(dispatch)

    def test_dispatch_receive_response(self):
        destination_queue = DummyQueue()
        test_id = "test"
        test_contents = haplib.ParsedMessage()
        test_contents.message_id = 1

        self.__dispatcher.attach_destination(destination_queue, test_id)
        acknowledge = testutils.get_priv_attr(self.__dispatcher,
                                              "__acknowledge")
        test_message = (test_id, test_contents.message_id)
        acknowledge(test_message)

        test_message = (test_id, test_contents)
        dispatch_queue = testutils.get_priv_attr(self.__dispatcher,
                                                 "__dispatch_queue")
        dispatch_queue.put(test_message)

        testutils.assertNotRaises(acknowledge, test_message)
        dispatch = testutils.get_priv_attr(self.__dispatcher, "__dispatch")
        testutils.assertNotRaises(dispatch)

    def test_daemonize(self):
        dispatcher = DispatcherForTest()
        testutils.assertNotRaises(dispatcher.daemonize)

    def test_call(self):
        dispatcher = DispatcherForTest()
        testutils.assertNotRaises(dispatcher)


class BaseMainPluginTestee(haplib.BaseMainPlugin):
    def __init__(self, **kwargs):
        haplib.BaseMainPlugin.__init__(self, **kwargs)

    @staticmethod
    def create(**kwargs):
        transporter_args = {"class": transporter.Transporter}
        plugin = BaseMainPluginTestee(**kwargs)
        plugin.setup(transporter_args)
        return plugin


class BaseMainPlugin(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        transporter_args = {"class": transporter.Transporter}
        cls.__main_plugin = haplib.BaseMainPlugin()
        cls.__main_plugin.setup(transporter_args)

    def test_hap_update_monitoring_server_info(self):
        test_params = {"serverId": None, "url": None, "nickName": None,
                       "userName": None, "password": None,
                       "pollingIntervalSec": None, "retryIntervalSec": None,
                       "extendedInfo": None, "type": None}
        testutils.assertNotRaises(self.__main_plugin.hap_update_monitoring_server_info,
                               test_params)

    def test_return_error(self):
        testutils.assertNotRaises(self.__main_plugin.hap_return_error, -32700, 1)

    def test_register_callback(self):
        testutils.assertNotRaises(self.__main_plugin.register_callback,
                               "test_code", "test_handler")

    def test_detect_implemented_procedures(self):
        detect_implemented_procedures = \
            testutils.get_priv_attr(self.__main_plugin,
                                    "__detect_implemented_procedures")
        detect_implemented_procedures()
        implemented_procedures = \
            testutils.get_priv_attr(self.__main_plugin,
                                    "__implemented_procedures")
        self.assertEquals({"exchangeProfile": self.__main_plugin.hap_exchange_profile,
                           "updateMonitoringServerInfo": self.__main_plugin.hap_update_monitoring_server_info},
                           implemented_procedures)

    def test_get_sender(self):
        sender = testutils.get_priv_attr(self.__main_plugin, "__sender")
        self.assertEquals(sender, self.__main_plugin.get_sender())

    def test_set_sender(self):
        test_sender = "test_sender"
        self.__main_plugin.set_sender(test_sender)
        sender = testutils.get_priv_attr(self.__main_plugin, "__sender")
        self.assertEquals(test_sender, sender)

    def test_get_dispatcher(self):
        dispatcher = testutils.get_priv_attr(self.__main_plugin, "__dispatcher")
        self.assertEquals(dispatcher, self.__main_plugin.get_dispatcher())

    def test_plugin_name(self):
        main = BaseMainPluginTestee.create(name="Love Sweets")
        self.assertEquals(main.get_plugin_name(), "Love Sweets")

    def test_plugin_name_default(self):
        main = BaseMainPluginTestee.create()
        self.assertEquals(main.get_plugin_name(), "BaseMainPluginTestee")

    def test_exchange_profile(self):
        self.__main_plugin._HapiProcessor__reply_queue = DummyQueue()
        self.__main_plugin.set_dispatch_queue(DummyQueue())
        self.__main_plugin._HapiProcessor__dispatcher = DispatcherForTest()
        try:
            self.__main_plugin.exchange_profile()
        except Exception as exception:
            self.assertEquals("Got", str(exception)[0:3])

    def test_hap_exchange_profile(self):
        self.__main_plugin._HapiProcessor__reply_queue = DummyQueue()
        self.__main_plugin.set_dispatch_queue(DummyQueue())
        self.__main_plugin._HapiProcessor__dispatcher = DispatcherForTest()
        try:
            self.__main_plugin.exchange_profile()
        except Exception as exception:
            self.assertEquals("Got", str(exception)[0:3])

    def test_request_exit(self):
        testutils.assertNotRaises(self.__main_plugin.request_exit)

    def test_is_exit_request(self):
        self.assertTrue(self.__main_plugin.is_exit_request(None))

    def test_start_receiver(self):
        testutils.assertNotRaises(self.__main_plugin.start_receiver)

    def test_start_dispatcher(self):
        self.__main_plugin._HapiProcessor__dispatcher = DispatcherForTest()
        testutils.assertNotRaises(self.__main_plugin.start_dispatcher)

    def test_optimize_server_procedures(self):
        test_procedures_dict = {"exchangeProfile": True,
                                "updateTriggers": True}
        test_procedures = ["exchangeProfile"]

        self.__main_plugin.__optimize_server_procedures(test_procedures_dict,
                                                        test_procedures)
        self.assertTrue(test_procedures_dict["exchangeProfile"])
        self.assertFalse(test_procedures_dict["updateTriggers"])

    def test_call(self):
        self.__main_plugin._HapiProcessor__reply_queue = DummyQueue()
        self.__main_plugin.set_dispatch_queue(DummyQueue())
        self.__main_plugin._HapiProcessor__dispatcher = DispatcherForTest()
        try:
            self.__main_plugin.exchange_profile()
        except Exception as exception:
            self.assertEquals("Got", str(exception)[0:3])


class BasePoller(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        transporter_args = {"class": transporter.Transporter}
        cls.sender = haplib.Sender(transporter_args)
        cls.poller = haplib.BasePoller(sender=cls.sender, process_id="test")

    def test_poll(self):
        testutils.assertNotRaises(self.poller.poll)

    def test_get_command_queue(self):
        command_queue = testutils.get_priv_attr(self.poller, "__command_queue")
        self.assertEquals(command_queue, self.poller.get_command_queue())

    def test_poll_setup(self):
        testutils.assertNotRaises(self.poller.poll_setup)

    def test_poll_hosts(self):
        testutils.assertNotRaises(self.poller.poll_hosts)

    def test_poll_host_groups(self):
        testutils.assertNotRaises(self.poller.poll_host_groups)

    def test_poll_host_group_membership(self):
        testutils.assertNotRaises(self.poller.poll_host_group_membership)

    def test_poll_triggers(self):
        testutils.assertNotRaises(self.poller.poll_triggers)

    def test_poll_events(self):
        testutils.assertNotRaises(self.poller.poll_events)

    def test_on_aboted_poll(self):
        testutils.assertNotRaises(self.poller.on_aborted_poll)

    def test_log_status(self):
        poller = haplib.BasePoller(sender=self.sender, process_id="test")
        log_time = testutils.get_priv_attr(poller, "__next_log_status_time")
        poller.log_status(haplib.ArmInfo())
        # check if the next time is update
        new_time = testutils.get_priv_attr(poller, "__next_log_status_time")
        self.assertGreater(new_time, log_time)
        # call again soon. __new_log_status_time should not be updated.
        new_time2 = testutils.get_priv_attr(poller, "__next_log_status_time")
        self.assertEquals(new_time2, new_time)

    def test_set_ms_info(self):
        ms_info = ("test_ms_info")
        self.poller.set_ms_info(ms_info)
        command_queue = testutils.get_priv_attr(self.poller, "__command_queue")
        q = testutils.get_priv_attr(command_queue, "__q")
        self.assertEquals((1, ms_info), q.get())

    def test_private_set_ms_info(self):
        set_ms_info = testutils.get_priv_attr(self.poller, "__set_ms_info")
        test_params = {"serverId": None, "url": None, "nickName": None,
                       "userName": None, "password": None,
                       "pollingIntervalSec": None, "retryIntervalSec": None,
                       "extendedInfo": None, "type": None}
        ms_info = haplib.MonitoringServerInfo(test_params)
        try:
            set_ms_info(ms_info)
            raise
        except hap.Signal:
            pass

    def test_call(self):
        class TestError:
            pass

        def __poll_in_try_block(self):
            raise TestError

        org_func = self.poller._BasePoller__poll_in_try_block
        self.poller._BasePoller__poll_in_try_block = __poll_in_try_block

        try:
            self.poller.__call__()
        except TestError:
            self.poller._BasePoller__poll_in_try_block = org_func
        except Exception:
            raise

    def test_poll_in_block(self):
        poll_in_try_block = testutils.get_priv_attr(self.poller,
                                                    "__poll_in_try_block")
        arm_info = haplib.ArmInfo()
        self.poller.set_dispatch_queue(DummyQueue())
        self.poller._HapiProcessor__reply_queue = DummyQueue()
        org_q = self.poller._BasePoller__command_queue._CommandQueue__q
        self.poller._BasePoller__command_queue._CommandQueue__q =\
                                                        DummyCommandQueue()
        testutils.assertNotRaises(poll_in_try_block, arm_info)
        self.poller._BasePoller__command_queue._CommandQueue__q = org_q


class ConnectorForTest(transporter.Transporter):
    def __init__(self, test_queue):
        self.__test_queue = test_queue
        self.__ms_flag = False

    def enable_ms_flag(self):
        self.__ms_flag = True

    def call(self, msg):
        response_id = json.loads(msg)["id"]
        if self.__ms_flag:
            result = {"extendedInfo": "exampleExtraInfo",
                      "serverId": 0,
                      "url": "http://example.com:80",
                      "type": 0,
                      "nickName": "exampleName",
                      "userName": "Admin",
                      "password": "examplePass",
                      "pollingIntervalSec": 30,
                      "retryIntervalSec": 10}
        else:
            result = "SUCCESS"

        pm = haplib.ParsedMessage()
        pm.message_dict = {"result": result, "id": response_id}
        pm.message_id = response_id
        self.__test_queue.put(pm)


class DummyQueue:
    def __init__(self):
        self.counter = int()

    def put(self, test_tuple, block=True):
        pass

    def join(self):
        pass

    def get(self, boolean, timeout_sec):
        if self.counter == 0:
            self.counter += 1
            return True
        elif self.counter == 1:
            return haplib.ParsedMessage()


class DummyCommandQueue:
    def get(self):
        pass

    def put(block=True):
        pass


class DispatcherForTest(haplib.Dispatcher):
    def __init__(self):
        pass

    def __call__(self):
        pass
