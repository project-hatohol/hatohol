#!/usr/bin/env python
# coding: UTF-8
#
#  Copyright (C) 2015-2016 Project Hatohol
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

"""
This module contains basic functions and class for HAP (Hatohol Arm Plugins)
and is supposed to be used both from haplib and transporter.
transporter and the sub class shall not import haplib. So the functions and
classes used in them have to be in this module.
"""

import logging
import logging.config
import logging.handlers
from logging import getLogger
import sys
import errno
import time
import traceback
import multiprocessing
import argparse
import ConfigParser
from hatohol import hapcommon

logger = getLogger("hatohol.hap:%s" % hapcommon.get_top_file_name())

def initialize_logger(parser=None):
    """
    Initialize logger for the module: hatohol.
    @param parser
    argparse.ArgumentParser object or None. If this parameter is not None,
    arguments for configuring logging parameters is added to the parser.
    """
    # This level is used until setup_logger() is called.
    # TODO: Shoud be configurable. For example, by environment variable
    handler = logging.handlers.SysLogHandler("/dev/log")
    fmt = "%(asctime)s %(levelname)8s [%(process)5d] %(name)s:%(lineno)d:  " \
          "%(message)s"
    formatter = logging.Formatter(fmt)
    handler.setFormatter(formatter)
    hatohol_logger = getLogger("hatohol")
    hatohol_logger.addHandler(handler)
    hatohol_logger.setLevel(logging.INFO)

    if parser is not None:
        choices = ("DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL")
        parser.add_argument("--log", dest="loglevel", choices=choices)
        parser.add_argument("--log-conf", dest="log_conf_file",
                            help="The path of the logging configuration file.")


def setup_logger(args):
    hatohol_logger = getLogger("hatohol")
    if args.log_conf_file is not None:
        logger.info("log_conf_file option was used. Remove default SysLogHandler and read the logging conf file.")
        hatohol_logger.removeHandler(logging.handlers.SysLogHandler())

    if args.loglevel:
        numeric_level = getattr(logging, args.loglevel.upper(), None)
        logger.debug(numeric_level)
    else:
        logger.debug("Unuse loglevel")
        return
    if not isinstance(numeric_level, int):
        raise ValueError('Invalid log level: %s' % loglevel)
    hatohol_logger.setLevel(numeric_level)


def handle_exception(raises=(SystemExit,)):
    """
    Logging exception information including back trace and return
    some information. This method is supposed to be used in 'except:' block.
    Note that if the exception class is Signal, this method doesn't log it.

    @raises
    A sequence of exceptionclass names. If the handling exception is one of
    it, this method just raises it again.

    @return
    A sequence of exception class and the instance of the handling exception.
    """
    (exctype, value, tb) = sys.exc_info()
    if exctype in raises:
        raise
    if exctype is not Signal:
        logger.error("Unexpected error: %s, %s\n%s" % \
                      (exctype, value, traceback.format_exc()))
    elif value.critical:
        logger.critical("Got critical signal.")
        raise
    return exctype, value


def _handle_ioerr(e, timeout, expired_time):
    if e.errno != errno.EINTR:
        raise
    if timeout is None:
        return None
    curr_time = time.time()
    if expired_time < curr_time:
        return 0
    return expired_time - curr_time


def MultiprocessingQueue(queue_class=multiprocessing.Queue):
    """
    The purpose of this function is to provide get() method with a retry
    when EINTR happens.

    """
    def __get(block=True, timeout=None):
        start_time = time.time()
        if timeout is not None:
            expired_time = start_time + timeout
        while True:
            try:
                return original_get(block, timeout)
            except IOError as e:
                timeout = _handle_ioerr(e, timeout, expired_time)

    q = queue_class()
    original_get = q.get
    q.get = __get
    return q


def MultiprocessingJoinableQueue():
    return MultiprocessingQueue(multiprocessing.JoinableQueue)


class Signal:
    """
    This class is supposed to raise as an exception in order to
    propagate some events and jump over stack frames.
    """

    def __init__(self, restart=False, critical=False):
        self.restart = restart
        self.critical = critical


class ConfigFileParser():
    def __init__(self, conf_path, remaining_args, parser):
        config_parser = ConfigParser.SafeConfigParser()
        config_parser.read(conf_path)
        self.item_dict = dict(config_parser.items("hap2"))
        self.parser = argparse.ArgumentParser(parents=[parser])
        self.group = self.parser.add_argument_group("Variables")
        self.remaining_args = remaining_args

    def add_argument(self, *args, **kwargs):
        default_value = None
        if kwargs.has_key("default"):
            default_value = kwargs.pop("default")

        for arg in args:
            arg_name = self.__remove_and_replace_hyphen(arg)
            if self.item_dict.has_key(arg_name):
                default_value = self.item_dict[arg_name]
                break
            else:
                continue

        if kwargs.has_key("type") and default_value:
            default_value = kwargs["type"](default_value)
        if kwargs.get("action")=="store_true" and default_value=="":
            default_value = True
        if kwargs.has_key("nargs") and default_value:
            default_value = default_value.split()
        if not default_value:
            default_value = None
        self.group.add_argument(default=default_value, *args, **kwargs)

    def parse_args(self):
        return self.parser.parse_args(self.remaining_args)

    def __remove_and_replace_hyphen(self, argument_name):
        while argument_name[0] == "-":
            argument_name = argument_name[1:]

        return argument_name.replace("-", "_")
