#!/usr/bin/env python
# coding: UTF-8
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

import os
import sys
import time
import signal
import logging
import logging.config
from logging import getLogger
import argparse
import subprocess
import commands
import ConfigParser

DEFAULT_ERROR_SLEEP_TIME = 10
logger = getLogger("hatohol." + "hap2_starter")

def create_pid_file(pid_dir, server_id):
    if not os.path.isdir(pid_dir): os.makedirs(pid_dir)

    with open("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id), "w") as file:
        file.write(str(os.getpid()))

    logger.info("PID file has been created.")

def check_existence_of_pid_file(pid_dir, server_id):
    return os.path.isfile("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id))

def remove_pid_file(pid_dir,server_id):
    os.remove("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id))
    logger.info("PID file has been removed.")

def setup_logger(hap_args):
    log_conf_path = None

    if "--log-conf" in hap_args:
        try:
            log_conf_path = hap_args[hap_args.index("--log-conf")+1]
            logging.config.fileConfig(log_conf_path)
            return
        except:
            raise Exception("Could not read log conf: %s" % log_conf_path)

    if "--config" in hap_args:
        config_parser = ConfigParser.SafeConfigParser()
        try:
            config_parser.read([hap_args[hap_args.index("--config")+1],])
            log_conf_path = config_parser.get("hap2", "log_conf")
        except:
            raise Exception("Could not parse log_conf from --config file")

        try:
            logging.config.fileConfig(log_conf_path)
            return
        except:
            raise Exception("Could not read --conf's log config file: %s"
                            % log_conf_path)

def check_existance_of_process_group(pgid):
    pgid = str(pgid)
    result = commands.getoutput("ps -eo pgid | grep -w "+ pgid)
    if pgid in result:
        return True
    return False


if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--plugin-path", required=True)
    parser.add_argument("--server-id")
    parser.add_argument("--pid-file-dir")
    self_args, hap_args = parser.parse_known_args()

    pid = os.fork()
    if pid > 0:
        sys.exit(0)

    setup_logger(hap_args)

    if self_args.server_id is None and self_args.pid_file_dir is not None:
        logger.error("If you use --pid-file-dir, you must use --server-id.")
        sys.exit(1)

    subprocess_args = ["python", self_args.plugin_path]
    subprocess_args.extend(hap_args)

    while True:
        hap = subprocess.Popen(subprocess_args, preexec_fn=os.setsid, close_fds=True)

        if self_args.pid_file_dir is not None and not hap.poll():
            create_pid_file(self_args.pid_file_dir, self_args.server_id)

        def signalHandler(signalnum, frame):
            os.killpg(hap.pid, signal.SIGKILL)
            logger.info("hap2_starter caught %d signal" % signalnum)
            sys.exit(0)

        signal.signal(signal.SIGINT, signalHandler)
        signal.signal(signal.SIGTERM, signalHandler)
        hap.wait()
        try:
            os.killpg(hap.pid, signal.SIGKILL)
            logger.info("%s process was finished" % self_args.plugin_path)
        except OSError:
            logger.info("%s process was finished" % self_args.plugin_path)

        if self_args.pid_file_dir is not None and \
            check_existence_of_pid_file(self_args.pid_file_dir, self_args.server_id):
            remove_pid_file(self_args.pid_file_dir, self_args.server_id)

        if check_existance_of_process_group(hap.pid):
            logger.error("Could not killed HAP2 processes. hap2_starter exit.")
            logger.error("You should kill the processes by yourself.")
            sys.exit(1)

        logger.info("Rerun after %d sec" % DEFAULT_ERROR_SLEEP_TIME)
        time.sleep(DEFAULT_ERROR_SLEEP_TIME)
