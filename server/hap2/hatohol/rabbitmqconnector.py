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

import logging
import pika
import haplib
from hatohol.transporter import Transporter

MAX_BODY_SIZE = 50000
MAX_FRAME_SIZE = 131072

class RabbitMQConnector(Transporter):
    def __init__(self):
        Transporter.__init__(self)
        self._channel = None
        self.__connection = None

    def setup(self, transporter_args):
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

        broker = transporter_args["amqp_broker"]
        port = transporter_args["amqp_port"]
        vhost = transporter_args["amqp_vhost"]
        queue_name = transporter_args["amqp_queue"]
        user_name = transporter_args["amqp_user"]
        password = transporter_args["amqp_password"]

        logging.debug("Called stub method: call().")
        self._queue_name = queue_name
        credentials = pika.credentials.PlainCredentials(user_name, password)

        conn_args = {}
        set_if_not_none(conn_args, "host", broker)
        set_if_not_none(conn_args, "port", port)
        set_if_not_none(conn_args, "virtual_host", vhost)
        set_if_not_none(conn_args, "credentials", credentials)
        set_if_not_none(conn_args, "frame_max", MAX_FRAME_SIZE)
        self.__setup_ssl(conn_args, transporter_args)

        param = pika.connection.ConnectionParameters(**conn_args)
        self.__connection = \
            pika.adapters.blocking_connection.BlockingConnection(param)
        self._channel = self.__connection.channel()
        self._channel.queue_declare(queue=queue_name)

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
                haplib.handle_exception()
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
            logging.warning("Receiver is not registered.")
            return
        receiver(self._channel, body)

    def __publish(self, msg):
        self._channel.basic_publish(exchange="", routing_key=self._queue_name,
                                    body=msg,
                                    properties=pika.BasicProperties(
                                        content_type="application/json"))

    @classmethod
    def define_arguments(cls, parser):
        parser.add_argument("--amqp-broker", type=str, default="localhost")
        parser.add_argument("--amqp-port", type=int, default=None)
        parser.add_argument("--amqp-vhost", type=str, default=None)
        parser.add_argument("--amqp-queue", type=str, default="hap2-queue")
        parser.add_argument("--amqp-user", type=str, default="hatohol")
        parser.add_argument("--amqp-password", type=str, default="hatohol")
        parser.add_argument("--amqp-ssl-key", type=str)
        parser.add_argument("--amqp-ssl-cert", type=str)
        parser.add_argument("--amqp-ssl-ca", type=str)

    @classmethod
    def parse_arguments(cls, args):
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
