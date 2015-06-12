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
import subprocess
from rabbitmqconnector import RabbitMQConnector

class TestRabbitMQConnector(unittest.TestCase):
    """
    Before executing this test, some setting for RabbitMQ is needed.
    See README in the same directory.
    """
    @classmethod
    def setUpClass(cls):
        cls.__broker = "localhost"
        cls.__port = None
        cls.__vhost = "test"
        cls.__queue_name = "test_queue"
        cls.__user_name  = "test_user"
        cls.__password   = "test_password"

    def test_setup(self):
        conn = RabbitMQConnector()
        conn.setup(self.__get_default_transporter_args())

    def test_call(self):
        TEST_BODY = "CALL TEST"
        self.__delete_test_queue()
        conn = self.__create_connected_connector()
        conn.call(TEST_BODY)
        self.assertEqual(self.__get_from_test_queue(), TEST_BODY)

    def test_reply(self):
        TEST_BODY = "REPLY TEST"
        self.__delete_test_queue()
        conn = self.__create_connected_connector()
        conn.reply(TEST_BODY)
        self.assertEqual(self.__get_from_test_queue(), TEST_BODY)

    def test_run_receive_loop_without_connect(self):
        conn = RabbitMQConnector()
        self.assertRaises(AssertionError, conn.run_receive_loop)

    def test_run_receive_loop(self):
        class Receiver():
            def __call__(self, channel, msg):
                self.msg = msg
                channel.stop_consuming()

        TEST_BODY = "FOO"
        self.__delete_test_queue()
        conn = RabbitMQConnector()
        conn.setup(self.__get_default_transporter_args())
        receiver = Receiver()
        conn.set_receiver(receiver)
        self.__publish(TEST_BODY)
        conn.run_receive_loop()
        self.assertEquals(receiver.msg, TEST_BODY)

    def __get_default_transporter_args(self):
        args = {"amqp_broker": self.__broker, "amqp_port": self.__port,
                "amqp_vhost":  self.__vhost, "amqp_queue": self.__queue_name,
                "amqp_user": self.__user_name, "amqp_password": self.__password}
        return args

    def __create_connected_connector(self):
        conn = RabbitMQConnector()
        conn.setup(self.__get_default_transporter_args())
        return conn

    def __execute(self, args):
        subproc = subprocess.Popen(args, stdout=subprocess.PIPE)
        output = subproc.communicate()[0]
        self.assertEquals(subproc.returncode, 0)
        return output

    def __build_broker_options(self):
        def append_if_not_none(li, key, val):
            if val is not None:
                li.append(key)
                li.append(val)

        opt = []
        append_if_not_none(opt, "-s", self.__broker)
        append_if_not_none(opt, "--port", self.__port)
        append_if_not_none(opt, "--vhost", self.__vhost)
        append_if_not_none(opt, "--username", self.__user_name)
        append_if_not_none(opt, "--password", self.__password)
        return opt

    def __publish(self, body):
        args = ["amqp-publish"]
        args.extend(self.__build_broker_options())
        args.extend(["-r", self.__queue_name, "-b", body])
        subprocess.Popen(args).communicate()

    def __get_from_test_queue(self):
        args = ["amqp-get"]
        args.extend(self.__build_broker_options())
        args.extend(["-q", self.__queue_name])
        return self.__execute(args)

    def __delete_test_queue(self):
        args = ["amqp-delete-queue"]
        args.extend(self.__build_broker_options())
        args.extend(["-q", self.__queue_name])
        subprocess.Popen(args).communicate()
