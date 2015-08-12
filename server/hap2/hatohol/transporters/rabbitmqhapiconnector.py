#!/usr/bin/env python
# coding: UTF-8
#
#  Copyright (C) 2015 Project Hatohol
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

from hatohol.rabbitmqconnector import RabbitMQConnector
from hatohol import transporter

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

transporter.Manager.register(RabbitMQHapiConnector)
