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
import os
import testutils
import subprocess
import hap
from rabbitmqconnector import RabbitMQConnector
from rabbitmqconnector import OverCapacity
import rabbitmqconnector

class TestRabbitMQConnector(unittest.TestCase):
    """
    Before executing this test, some setting for RabbitMQ is needed.
    See README in the same directory.
    """
    @classmethod
    def setUpClass(cls):
        cls.__broker = "localhost"
        port = os.getenv("RABBITMQ_NODE_PORT")
        cls.__port = None
        if port is not None:
            cls.__port = int(port)
        cls.__vhost = "test"
        cls.__queue_name = "test_queue"
        cls.__user_name = "test_user"
        cls.__password = "test_password"
        cls.__ssl_key = None
        cls.__ssl_cert = None
        cls.__ssl_ca = None

    def __connect(self):
        conn = RabbitMQConnector()
        conn.setup(self.__get_default_transporter_args())
        return conn

    def test_setup(self):
        self.__connect()

    def test_close(self):
        conn = self.__connect()
        conn.close()

    def test__setup_ssl(self):
        conn = RabbitMQConnector()
        target_func = testutils.returnPrivObj(conn, "__setup_ssl", "RabbitMQConnector")
        conn_args = {}
        transporter_args = self.__get_default_transporter_args()
        target_func(conn_args, transporter_args)
        self.assertNotIn("ssl", conn_args)
        self.assertNotIn("ssl_options", conn_args)

        transporter_args["amqp_ssl_key"] = "/foo/key"
        transporter_args["amqp_ssl_cert"] = "/foo/cert"
        transporter_args["amqp_ssl_ca"] = "/kamo/ca"
        conn_args = {}
        target_func(conn_args, transporter_args)
        self.assertTrue(conn_args["ssl"])
        ssl_options = conn_args["ssl_options"]
        self.assertEquals(ssl_options["keyfile"], "/foo/key")
        self.assertEquals(ssl_options["certfile"], "/foo/cert")
        self.assertEquals(ssl_options["ca_certs"], "/kamo/ca")

    def test_call(self):
        TEST_BODY = "CALL TEST"
        self.__delete_test_queue()
        conn = self.__create_connected_connector()
        conn.call(TEST_BODY)
        self.assertEqual(self.__get_from_test_queue(), TEST_BODY)

    def test_call_failed(self):
        TEST_BODY = str()
        for num in range(0, rabbitmqconnector.MAX_BODY_SIZE+1):
            TEST_BODY += "a"

        self.__delete_test_queue()
        conn = self.__create_connected_connector()
        try:
            conn.call(TEST_BODY)
            raise
        except OverCapacity:
            pass

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

    def test__publish_with_exception(self):
        conn = RabbitMQConnector()
        testutils.set_priv_attr(conn, "__publish_raw", None)
        target_func = testutils.returnPrivObj(conn, "__publish")
        with self.assertRaises(hap.Signal) as cm:
            target_func("msg")
        self.assertTrue(cm.exception.critical)

    def __get_default_transporter_args(self):
        args = {"amqp_broker": self.__broker, "amqp_port": self.__port,
                "amqp_vhost":  self.__vhost, "amqp_queue": self.__queue_name,
                "amqp_user": self.__user_name, "amqp_password": self.__password,
                "amqp_ssl_key": self.__ssl_key, "amqp_ssl_cert": self.__ssl_cert,
                "amqp_ssl_ca": self.__ssl_ca}
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

    def __build_broker_url(self):
        port = ""
        if self.__port is not None:
            port = ":%d" % self.__port
        return "amqp://%s:%s@%s%s/%s" % (self.__user_name, self.__password,
                                         self.__broker, port, self.__vhost)

    def __build_broker_options(self):
        if os.getenv("RABBITMQ_CONNECTOR_TEST_WORKAROUND") is not None:
            return ["-u", self.__build_broker_url()]

        def append_if_not_none(li, key, val):
            if val is not None:
                li.append(key)
                li.append(val)

        opt = []
        server = self.__broker
        if server is not None and self.__port is not None:
            server += ":%d" % self.__port
        append_if_not_none(opt, "--server", server)
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
