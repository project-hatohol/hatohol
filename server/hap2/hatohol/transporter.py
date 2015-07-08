#! /usr/bin/env python
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

DIR_BOTH = 0
DIR_SEND = 1
DIR_RECV = 2

class Transporter:
    """
    An abstract class for transportation of RPC messages for HAPI-2.0.
    """
    def __init__(self):
        self.__receiver = None

    @classmethod
    def define_arguments(cls, parser):
        """
        Define arguments for argparser.
        @param parser A parser object otabined by argparse.ArgumentParser()
        """
        pass

    @classmethod
    def parse_arguments(cls, args):
        """
        Parse arguements.
        @param args A parsed argument object returned by parser.parse_args()
        @return A dictionary in which parsed results will be being added.
        """
        return {}

    def setup(self, transporter_args):
        """
        Setup a transporter. Some transporter implementation connects to
        other side in this method.
        @param transporter_args
        A dictionary that contains parameters for the setup.
        The following keys shall be included.
        - class     A transporter class.
        - direction A communication direction that is one of
                    DIR_SEND, DIR_RECV, DIR_BOTH.
        Other elements, that should be included in, are depending on
        the implementation.
        """
        pass

    def close(self):
        """
        Close connection.
        """
        pass

    def call(self, msg):
        """
        Call a RPC.
        @param mas A message to be sent.
        """
        logging.debug("Called stub method: call().")

    def reply(self, msg):
        """
        Replay a message to a caller.
        @param msg A reply message.
        """
        logging.debug("Called stub method: reply().")

    def set_receiver(self, receiver):
        """
        Register a receiver method.
        @receiver A receiver method.
        """
        self.__receiver = receiver

    def get_receiver(self):
        """
        @return
        The registerd receiver method.
        If no receiver is registered, None is returned.
        """
        return self.__receiver

    def run_receive_loop(self):
        """
        Start receiving. When a message arrives, handlers registered by
        add_receiver() are called. This method doesn't return.
        """
        pass


class Factory:
    @classmethod
    def create(cls, transporter_args):
        """
        Create a transporter object.
        @param transporter_args
        A dictionary that contains parameters.
        This method create an instance of transporter_args['class'].
        Then call setup() of the crated object.
        @return A created transporter object.
        """
        obj = transporter_args["class"]()
        obj.setup(transporter_args)
        return obj
