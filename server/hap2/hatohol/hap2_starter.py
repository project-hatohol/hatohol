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

DEFAULT_ERROR_SLEEP_TIME = 10
logger = getLogger("hatohol." + "hap2_starter")

def create_pid_file(pid_dir, server_id, hap_pid):
    if not os.path.isdir(pid_dir): os.makedirs(pid_dir)

    with open("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id), "w") as file:
        file.writelines([str(os.getpid()), "\n", str(hap_pid)])

    logger.info("PID file has been created.")

def check_existence_of_pid_file(pid_dir, server_id):
    return os.path.isfile("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id))

def remove_pid_file(pid_dir,server_id):
    os.remove("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id))
    logger.info("PID file has been removed.")

def setup_logger(hap_args):
    if "--log-conf" in hap_args:
        logging.config.fileConfig(hap_args[hap_args.index("--log-conf")+1])

def check_existance_of_process_group(pgid):
    pgid = str(pgid)
    result = commands.getoutput("ps -eo pgid | grep -w "+ pgid)
    if pgid in result:
        return True
    return False

if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--plugin-path")
    parser.add_argument("--server-id")
    parser.add_argument("--pid-file-dir")
    self_args, hap_args = parser.parse_known_args()

    pid = os.fork()
    if pid > 0:
        sys.exit(0)

    setup_logger(hap_args)
    subprocess_args = ["python", self_args.plugin_path]
    subprocess_args.extend(hap_args)

    while True:
        hap = subprocess.Popen(subprocess_args, preexec_fn=os.setsid, close_fds=True)

        if self_args.server_id is not None and not hap.poll():
            create_pid_file(self_args.pid_file_dir,
                            self_args.server_id, hap.pid)

        hap.wait()
        try:
            os.killpg(hap.pid, signal.SIGKILL)
        except OSError:
            logger.info("%s process was finished" % self_args.plugin_path)

        if check_existance_of_process_group(pgid):
            logger.error("Can not killed HAP2 processes. hap2_starter exit.")
            sys.exit(1)

        if self_args.server_id is not None and \
            check_existence_of_pid_file(self_args.pid_file_dir, self_args.server_id):
            remove_pid_file(self_args.pid_file_dir, self_args.server_id)

        logger.info("Rerun after %d sec" % DEFAULT_ERROR_SLEEP_TIME)
        time.sleep(DEFAULT_ERROR_SLEEP_TIME)
