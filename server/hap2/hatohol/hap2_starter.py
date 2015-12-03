#!/usr/bin/env python
# coding: UTF-8
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

import os
import sys
import time
import signal
import logging
import argparse
import subprocess
from hatohol import haplib

DEFAULT_ERROR_SLEEP_TIME = 10
logger = logging.Logger(__name__)

def create_pid_file(pid_dir, server_id, hap_pid):
    with open("%s/hatohol-arm-plugin-%s" % (pid_dir, server_id), "w") as file:
        file.writelines([str(os.getpid()), "\n", str(hap_pid)])

def remove_pid_file(pid_dir,server_id):
    subprocess.call("rm %s/hatohol-arm-plugin-%s" % (pid_dir, server_id))

if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--plugin-path")
    parser.add_argument("--server-id")
    parser.add_argument("--pid-file-dir")
    self_args, hap_args = parser.parse_known_args()

    pid = os.fork()
    if pid > 0:
        sys.exit(0)

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

        if self_args.server_id is not None:
            remove_pid_file(self_args.pid_file_dir, self_args.server_id)

        logger.info("Rerun after %d sec" % DEFAULT_ERROR_SLEEP_TIME)
        time.sleep(DEFAULT_ERROR_SLEEP_TIME)
