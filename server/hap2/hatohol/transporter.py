#! /usr/bin/env python
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

from logging import getLogger
import sys
import os
import imp

DIR_BOTH = 0
DIR_SEND = 1
DIR_RECV = 2

logger = getLogger(__name__)

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
        logger.debug("Called stub method: call().")

    def reply(self, msg):
        """
        Replay a message to a caller.
        @param msg A reply message.
        """
        logger.debug("Called stub method: reply().")

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


class Manager(object):

    __transporters = {}

    def __init__(self, default_transporter):
        self.__default_transporter = default_transporter
        self.__import_modules()

    def define_arguments(self, parser):
        parser.add_argument("--transporter", type=str,
                            default=self.__default_transporter)

        for transporter_class in self.__transporters.values():
            transporter_class.define_arguments(parser)

    def find(self, name):
        """
        Find transporter from the registered list.
        @param name a name of the transporter
        @return A class of the found transporter or None if not.
        """
        return self.__transporters.get(name)

    def __import_modules(self):
        for path in sys.path:
            self.__import_from(path)

    def __import_from(self, path):
        for mod_name in ("hatohol", "transporters"):
            mod = self.import_module(mod_name, path)
            if mod is None:
                return None
            path = mod.__path__
        logger.info("Loaded transporter module: %s", path[0])

    @classmethod
    def register(cls, transporter_cls):
        # NOTE: Currently a unique name is required.
        name = transporter_cls.__name__
        cls.__transporters[name] = transporter_cls
        logger.info("Registered: %s" % name)

    @classmethod
    def import_module(cls, name, path):
        """
        Import a module that is in the same directory.
        @param name A module name
        @param path A module path or a list of them
        @return A module object is returned if successful, or None.
        """
        if not isinstance(path, list):
            path = [path]
        try:
             (file, pathname, descr) = imp.find_module(name, path)
        except ImportError:
            return None
        return imp.load_module("", file, pathname, descr)
