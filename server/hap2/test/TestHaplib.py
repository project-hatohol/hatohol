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
        # This is not smart. The processing won't be completed within 'duration'
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

        print key_map
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


class RabbitMQHapiConnector(unittest.TestCase):
    def test_setup(self):
        port = os.getenv("RABBITMQ_NODE_PORT")
        amqp_address = os.getenv("RABBITMQ_NODE_ADDRESS")
        transporter_args = {"direction": transporter.DIR_SEND,
                            "amqp_broker": amqp_address,
                            "amqp_port": port,
                            "amqp_vhost": "test",
                            "amqp_queue": "test_queue",
                            "amqp_user": "test_user",
                            "amqp_password": "test_password"}
        rabbitmq_connector = haplib.RabbitMQHapiConnector()
        common.assertNotRaises(rabbitmq_connector.setup, transporter_args)


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
