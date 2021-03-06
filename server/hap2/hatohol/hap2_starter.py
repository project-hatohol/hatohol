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
from datetime import datetime

DEFAULT_ERROR_SLEEP_TIME = 3
RETRY_TIME_RANGE = 300
RETRY_REPEAT_COUNT = 5
logger = getLogger("hatohol." + "hap2_starter")

def create_pid_file(pid_dir, server_id):
    if not os.path.isdir(pid_dir): os.makedirs(pid_dir)

    with open("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id), "w") as file:
        file.write(str(os.getpid()))

    if check_existence_of_pid_file(pid_dir, server_id):
        logger.info("PID file has been created")
    else:
        logger.error("Could not create PID file. Finish HAP2")
        sys.exit(1)

def check_existence_of_pid_file(pid_dir, server_id):
    return os.path.isfile("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id))

def remove_pid_file(pid_dir,server_id):
    try:
        os.remove("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id))
        logger.info("PID file has been removed.")
    except OSError:
        logger.error("PID file does not exist on %s." % pid_dir)

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

def kill_hap2_processes(pgid, plugin_path):
    try:
        os.killpg(pgid, signal.SIGKILL)
        logger.info("%s process was finished" % plugin_path)
    except OSError:
        logger.info("%s process was finished" % plugin_path)

def total_seconds(timedelta):
    return int((timedelta.microseconds + 0.0 +
                (timedelta.seconds +
                    timedelta.days * 24 * 3600) * 10 ** 6) / 10 ** 6)


if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--plugin-path", required=True)
    parser.add_argument("--server-id")
    parser.add_argument("--pid-file-dir")
    self_args, hap_args = parser.parse_known_args()
    setup_logger(hap_args)

    if self_args.server_id is None and self_args.pid_file_dir is not None:
        logger.error("If you use --pid-file-dir, you must use --server-id.")
        sys.exit(1)

    if os.fork() > 0:
        sys.exit(0)

    if self_args.pid_file_dir is not None:
        create_pid_file(self_args.pid_file_dir, self_args.server_id)

    subprocess_args = ["python", self_args.plugin_path]
    subprocess_args.extend(hap_args)

    finish_counter = int()
    base_time = datetime.now()
    while True:
        hap = subprocess.Popen(subprocess_args, preexec_fn=os.setsid, close_fds=True)

        def signalHandler(signalnum, frame):
            logger.info("hap2_starter caught %d signal" % signalnum)
            kill_hap2_processes(hap.pid, self_args.plugin_path)
            logger.info("hap2_starter killed all HAP2 processes")
            if self_args.pid_file_dir is not None and \
                check_existence_of_pid_file(self_args.pid_file_dir, self_args.server_id):
                remove_pid_file(self_args.pid_file_dir, self_args.server_id)
            sys.exit(0)

        signal.signal(signal.SIGTERM, signalHandler)
        hap.wait()

        if total_seconds(datetime.now()-base_time) < RETRY_TIME_RANGE:
            finish_counter += 1
            logger.debug("Increment finish counter.")
        else:
            base_time = datetime.now()
            finish_counter = 1
            logger.debug("Reset finish counter.")

        kill_hap2_processes(hap.pid, self_args.plugin_path)

        if check_existance_of_process_group(hap.pid):
            logger.error("Could not killed HAP2 processes. hap2_starter exit.")
            logger.error("You should kill the processes by yourself.")
            sys.exit(1)

        if finish_counter >= RETRY_REPEAT_COUNT:
            logger.error("HAP2 abnormal termination: %d times within 60 seconds." % RETRY_REPEAT_COUNT)
            sys.exit(1)

        logger.info("Rerun after %d sec" % DEFAULT_ERROR_SLEEP_TIME)
        time.sleep(DEFAULT_ERROR_SLEEP_TIME)
