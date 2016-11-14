#! /usr/bin/env python
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

import argparse
from logging import getLogger
import signal
import os
from hatohol import hap
from hatohol import haplib
from hatohol import transporter
from hatohol import hapcommon
from hatohol import autotools_vars
import logging

logger = getLogger("hatohol.standardhap:%s" % hapcommon.get_top_file_name())

class StandardHap:
    def __init__(self, default_transporter="RabbitMQHapiConnector"):
        parser = argparse.ArgumentParser(add_help=False)
        group = parser.add_argument_group("Config")

        help_msg = """
        A configuration file.
        Other command line arguments override items in the config. file.
        """
        group.add_argument("--config", default=None, help=help_msg)
        args, remaining_args = parser.parse_known_args()

        config_path = None
        if args.config:
            config_path = args.config
        else:
            default_conf_path = autotools_vars.SYSCONFDIR + "/hatohol/hap2.conf"
            if os.path.isfile(default_conf_path):
                config_path = default_conf_path
        if config_path:
            parser = hap.ConfigFileParser(config_path, remaining_args, parser)
        else:
            #The line enable help message.
            parser = argparse.ArgumentParser(parents=[parser])

        hap.initialize_logger(parser)
        if config_path:
            logger.info("Configuration file: %s" % config_path)
        parser.add_argument("-p", "--disable-poller", action="store_true")
        parser.add_argument("--polling-targets", nargs="*",
                            choices=haplib.BasePoller.ACTIONS,
                            default=haplib.BasePoller.ACTIONS,
                            help="Names of polloing action. Default is all.")

        help_msg = """
            Minimum status logging interval in seconds.
            The actual interval depends on the implementation and is
            typically larger than this parameter.
            """
        parser.add_argument("--status-log-interval", type=int, default=600,
                            help=help_msg)
        self.__transporter_manager = transporter.Manager(default_transporter)
        self.__transporter_manager.define_arguments(parser)

        self.__parser = parser
        self.__main_plugin = None
        self.__poller = None

    def get_argument_parser(self):
        return self.__parser

    def get_main_plugin(self):
        return self.__main_plugin

    def get_poller(self):
        return self.__poller

    """
    An abstract method to create main plugin process.
    A sub class shall implement this method, or the default implementation
    raises AssertionError.

    @return
    A main plugin. The class shall be callable.
    """
    def create_main_plugin(self, *args, **kwargs):
        raise NotImplementedError

    """
    An abstract method to create poller plugin process.

    @return
    An poller plugin. The class shall be callable.
    If this method returns None, no poller process is created.
    The default implementation returns None.
    """
    def create_poller(self, *args, **kwargs):
        return None

    def on_parsed_argument(self, args):
        pass

    def on_got_monitoring_server_info(self, ms_info):
        self.get_main_plugin().set_ms_info(ms_info)

        poller = self.get_poller()
        if poller is not None:
            poller.set_ms_info(ms_info)

    def __setup(self):
        args = self.__parser.parse_args()
        hap.setup_logger(args)

        self.on_parsed_argument(args)
        return args

    def run(self):
        self()

    def __call__(self):
        args = self.__setup()
        try:
            self.__run(args)
        except:
            raises = (KeyboardInterrupt, AssertionError, SystemExit)
            exctype, value = hap.handle_exception(raises=raises)

        self.enable_handling_sigchld(False)

    def __create_poller(self, sender, dispatcher, **kwargs):
        poller = self.create_poller(sender=sender, process_id="Poller",
                                    **kwargs)
        if poller is None:
            return
        logger.info("created poller plugin.")
        dispatcher.attach_destination(poller.get_reply_queue(), "Poller")
        poller.set_dispatch_queue(dispatcher.get_dispatch_queue())
        return poller

    def enable_handling_sigchld(self, enable=True):
        def handler(signum, frame):
            logger.warning("Got SIGCHLD")
            raise hap.Signal()
        if enable:
            _handler = handler
        else:
            _handler = signal.SIG_DFL
        signal.signal(signal.SIGCHLD, _handler)

    def enable_handling_terminate_signal(self):
        def handler(signum, frame):
            logger.warning("Got SIGTERM")
            raise SystemExit()
        signal.signal(signal.SIGTERM, handler)

    def __run(self, args):
        self.enable_handling_sigchld()
        self.enable_handling_terminate_signal()
        logger.info("Transporter: %s" % args.transporter)
        transporter_class = self.__transporter_manager.find(args.transporter)
        if transporter_class is None:
            logger.critical("Not found transporter: %s" % args.transporter)
            raise SystemExit()
        transporter_args = {"class": transporter_class}
        transporter_args.update(transporter_class.parse_arguments(args))

        self.__main_plugin = self.create_main_plugin()
        self.__main_plugin.setup(transporter_args)
        self.__main_plugin.register_callback(
            haplib.BaseMainPlugin.CB_UPDATE_MONITORING_SERVER_INFO,
            self.on_got_monitoring_server_info)
        logger.info("created main plugin.")

        if args.disable_poller:
            logger.info("Disabled: poller plugin.")
        else:
            kwargs = {
                "status_log_interval": args.status_log_interval,
                "polling_targets": args.polling_targets,
            }
            poller_sender = haplib.Sender(transporter_args)
            self.__poller = self.__create_poller(
                                poller_sender,
                                self.__main_plugin.get_dispatcher(),
                                **kwargs)

        self.__main_plugin.start_dispatcher()
        logger.info("started dispatcher process.")
        self.__main_plugin.start_receiver()
        logger.info("started receiver process.")

        self.__main_plugin.exchange_profile()
        logger.info("exchanged profile.")

        ms_info = self.__main_plugin.get_monitoring_server_info()
        logger.info("got monitoring server info.")
        self.on_got_monitoring_server_info(ms_info)

        if self.__poller is not None:
            self.__poller.daemonize("Poller")
            logger.info("started poller plugin.")

        self.__main_plugin()
