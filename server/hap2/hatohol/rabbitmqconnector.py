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

from logging import getLogger
import os
import pika
from pika import exceptions
import traceback
import hap
from hatohol.transporter import Transporter
from hatohol import hapcommon

logger = getLogger("hatohol.rabbitmqconnector:%s" % hapcommon.get_top_file_name())

MAX_BODY_SIZE = 50000
MAX_FRAME_SIZE = 131072
DEFAULT_HEARTBEAT_TIME = 60

class RabbitMQConnector(Transporter):
    HAPI_AMQP_PASSWORD_ENV_NAME = "HAPI_AMQP_PASSWORD"
    def __init__(self):
        Transporter.__init__(self)
        self._channel = None
        self.__connection = None
        self.__transporter_args = None

    def _connect(self, transporter_args=None):
        """
        @param transporter_args
        The following keys shall be included.
        - amqp_broker     A broker IP or FQDN.
        - amqp_port       A broker port.
        - amqp_vhost      A virtual host.
        - amqp_queue      A queue name.
        - amqp_user       A user name.
        - amqp_password   A password.
        - amqp_ssl_key    A file path to the key file (pem).
        - amqp_ssl_cert   A file path to client certificate (pem).
        - amqp_ssl_ca     A file path to CA certificate (pem).
        """

        def set_if_not_none(kwargs, key, val):
            if val is not None:
                kwargs[key] = val

        if transporter_args:
            self.__transporter_args = transporter_args
        else:
            transporter_args = self.__transporter_args

        broker = transporter_args["amqp_broker"]
        port = transporter_args["amqp_port"]
        vhost = transporter_args["amqp_vhost"]
        queue_name = transporter_args["amqp_queue"]
        user_name = transporter_args["amqp_user"]
        password = transporter_args["amqp_password"]

        logger.debug("Called stub method: call().")
        self._queue_name = queue_name
        credentials = pika.credentials.PlainCredentials(user_name, password)

        conn_args = {}
        set_if_not_none(conn_args, "host", broker)
        set_if_not_none(conn_args, "port", port)
        set_if_not_none(conn_args, "virtual_host", vhost)
        set_if_not_none(conn_args, "credentials", credentials)
        set_if_not_none(conn_args, "frame_max", MAX_FRAME_SIZE)

        # This check is to avoid the error in TravisCI with message below.
        #
        #       param = pika.connection.ConnectionParameters(**conn_args)
        # TypeError: __init__() got an unexpected keyword argument 'heartbeat_interval'
        #
        if hasattr(pika.connection.ConnectionParameters,
                   "DEFAULT_HEARTBEAT_INTERVAL"):
            set_if_not_none(conn_args, "heartbeat_interval",
                            DEFAULT_HEARTBEAT_TIME)
        else:
           logger.warning("skip: setting heartbeat_interval.")

        self.__setup_ssl(conn_args, transporter_args)

        param = pika.connection.ConnectionParameters(**conn_args)
        self.__connection = \
            pika.adapters.blocking_connection.BlockingConnection(param)
        self._channel = self.__connection.channel()
        self._channel.queue_declare(queue=queue_name)

    def setup(self, transporter_args):
        self._connect(transporter_args)

    def __setup_ssl(self, conn_args, transporter_args):
        ssl_key = transporter_args["amqp_ssl_key"]
        ssl_cert = transporter_args["amqp_ssl_cert"]
        ssl_ca = transporter_args["amqp_ssl_ca"]

        if ssl_key is None:
            return
        conn_args["ssl"] = True
        conn_args["ssl_options"] = {
            "keyfile": ssl_key,
            "certfile": ssl_cert,
            "ca_certs": ssl_ca,
        }

    def close(self):
        if self.__connection is not None:
            try:
                self.__connection.close()
            except:
                # On some condition such as Ubuntu 14.04, the above close()
                # raises an exception when the rabbitmq-server is stopped.
                hap.handle_exception()

            self.__connection = None

    def call(self, msg):
        if len(msg) > MAX_BODY_SIZE:
            raise OverCapacity("The message size over the max capacity of pika module.")
        self.__publish(msg)

    def reply(self, msg):
        self.__publish(msg)

    def run_receive_loop(self):
        assert self._channel != None

        self._channel.basic_consume(self.__consume_handler,
                                    queue=self._queue_name, no_ack=True)
        self._channel.start_consuming()

    def __consume_handler(self, ch, method, properties, body):
        receiver = self.get_receiver()
        if receiver is None:
            logger.warning("Receiver is not registered.")
            return
        logger.debug(body)
        receiver(self._channel, body)

    def __publish(self, msg):
        try:
            logger.debug(msg)
            self.__publish_raw(msg)
        except:
            logger.error(traceback.format_exc())
            raise hap.Signal(critical=True)

    def __publish_raw(self, msg):
        publish_argument = {
                            "exchange": "",
                            "routing_key": self._queue_name,
                            "body": msg,
                            "properties": pika.BasicProperties(
                                        content_type="application/json")
                            }
        try:
            self._channel.basic_publish(**publish_argument)
        except exceptions.ConnectionClosed:
            self._connect()
            self._channel.basic_publish(**publish_argument)
            logger.debug("Reconnect to RabbitMQ broker")

    @classmethod
    def define_arguments(cls, parser):

        password_help = \
        """
        A password for the AMQP connection. This option is deprecated for a
        security reason. You should specify the password with the environment
        variable: %s instead.
        """ % cls.HAPI_AMQP_PASSWORD_ENV_NAME

        parser.add_argument("--amqp-broker", type=str, default="localhost")
        parser.add_argument("--amqp-port", type=int, default=None)
        parser.add_argument("--amqp-vhost", type=str, default=None)
        parser.add_argument("--amqp-queue", type=str, default="hap2-queue")
        parser.add_argument("--amqp-user", type=str, default="hatohol")
        parser.add_argument("--amqp-password", type=str, default="hatohol",
                            help=password_help)
        parser.add_argument("--amqp-ssl-key", type=str)
        parser.add_argument("--amqp-ssl-cert", type=str)
        parser.add_argument("--amqp-ssl-ca", type=str)

    @classmethod
    def parse_arguments(cls, args):
        args.amqp_password = \
            os.getenv(cls.HAPI_AMQP_PASSWORD_ENV_NAME, args.amqp_password)

        return {"amqp_broker": args.amqp_broker,
                "amqp_port": args.amqp_port,
                "amqp_vhost": args.amqp_vhost,
                "amqp_queue": args.amqp_queue,
                "amqp_user": args.amqp_user,
                "amqp_password": args.amqp_password,
                "amqp_ssl_key": args.amqp_ssl_key,
                "amqp_ssl_cert": args.amqp_ssl_cert,
                "amqp_ssl_ca": args.amqp_ssl_ca}


class OverCapacity(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)
