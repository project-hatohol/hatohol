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
from transporter import Transporter

class RabbitMQConnector(Transporter):

    def __init__(self):
        Transporter.__init__(self)
        self._channel = None

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

        logging.debug("Called stub method: call().");
        self._queue_name = queue_name
        credentials = pika.credentials.PlainCredentials(user_name, password)

        conn_args = {}
        set_if_not_none(conn_args, "host", broker)
        set_if_not_none(conn_args, "port", port)
        set_if_not_none(conn_args, "virtual_host", vhost)
        set_if_not_none(conn_args, "credentials", credentials)
        param = pika.connection.ConnectionParameters(**conn_args)
        connection = pika.adapters.blocking_connection.BlockingConnection(param)
        self._channel = connection.channel()
        self._channel.queue_declare(queue=queue_name)

    def call(self, msg):
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
                                    body=msg)

    @classmethod
    def define_arguments(cls, parser):
        parser.add_argument("--amqp-broker", type=str, default="localhost")
        parser.add_argument("--amqp-port", type=int, default=None)
        parser.add_argument("--amqp-vhost", type=str, default=None)
        parser.add_argument("--amqp-queue", type=str, default="hap2-queue")
        parser.add_argument("--amqp-user", type=str, default="hatohol")
        parser.add_argument("--amqp-password", type=str, default="hatohol")

    @classmethod
    def parse_arguments(cls, args):
        return {"amqp_broker": args.amqp_broker,
                "amqp_port": args.amqp_port,
                "amqp_vhost": args.amqp_vhost,
                "amqp_queue": args.amqp_queue,
                "amqp_user": args.amqp_user,
                "amqp_password": args.amqp_password}
