#! /usr/bin/env python
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
import subprocess
from logging import getLogger
import multiprocessing
import datetime
import json
import re
import time
import signal
from hatohol import hap
from hatohol import haplib
from hatohol import standardhap
from hatohol import hapcommon

logger = getLogger("hatohol.hap2_fluentd:%s" % hapcommon.get_top_file_name())

class Hap2FluentdMain(haplib.BaseMainPlugin):

    # Ex.) 2016-03-03 12:11:17 +0900 hatohol.hoge
    __re_expected_msg = re.compile(
      r"^\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d \+\d\d\d\d [^\[]+:")

    def __init__(self, *args, **kwargs):
        haplib.BaseMainPlugin.__init__(self)

        self.__manager = None
        self.__default_host = "UNKNOWN"
        self.__default_type = "UNKNOWN"
        self.__default_status = "UNKNOWN"
        self.__default_severity = "UNKNOWN"

        self.__message_key = "message"
        self.__host_key = "host"
        self.__type_key = "type"
        self.__severity_key = "severity"
        self.__status_key = "status"

        self.__arm_info = haplib.ArmInfo()

    def set_arguments(self, args):
        # TODO by 15.09: Support escape of space characters
        self.__launch_args = args.fluentd_launch.split(" ")
        self.__accept_tag_reg = args.tag
        self.__accept_tag_pattern = re.compile(self.__accept_tag_reg)
        self.__status_log_interval = args.status_log_interval

    def set_ms_info(self, ms_info):
        self.__ms_info = ms_info
        if self.__manager is not None:
            return
        manager = multiprocessing.Process(target=self.__fluentd_manager_main)
        self.__manager = manager
        manager.daemon = True
        manager.start()

    def __setup_log_status_timer(self):
        def __timer_handler(signum, frame):
            logger.info(self.__arm_info.get_summary())

        signal.signal(signal.SIGALRM, __timer_handler)
        interval = self.__status_log_interval
        signal.setitimer(signal.ITIMER_REAL, interval, interval)

    def __fluentd_manager_main(self):
        self.__setup_log_status_timer()
        while True:
            try:
                self.__fluentd_manager_main_in_try_block()
            except:
                hap.handle_exception()
                self.__arm_info.fail()
                time.sleep(self.__ms_info.retry_interval_sec)

    def __fluentd_manager_main_in_try_block(self):
        logger.info("Started fluentd manger process.")
        fluentd = subprocess.Popen(self.__launch_args, stdout=subprocess.PIPE)
        while True:
            line = fluentd.stdout.readline()
            if len(line) == 0:
                logger.warning("The child process seems to have gone away.")
                fluentd.kill() # To make sure that the child terminates
                raise hap.Signal()
            timestamp, tag, raw_msg = self.__parse_line(line)
            if timestamp is None:
                continue
            if not self.__accept_tag_pattern.match(tag):
                continue
            self.__put_event(timestamp, tag, raw_msg)
            self.__arm_info.success()

    def __put_event(self, timestamp, tag, raw_msg):
        event_id = self.__generate_event_id()
        try:
            msg = json.loads(raw_msg)
        except:
            msg = {}

        brief = msg.get(self.__message_key, raw_msg)
        host = msg.get(self.__host_key, self.__default_host)

        hapi_event_type = self.__get_parameter(msg, self.__type_key,
                                               self.__default_type,
                                               haplib.EVENT_TYPES)
        hapi_status = self.__get_parameter(msg, self.__status_key,
                                           self.__default_status,
                                           haplib.TRIGGER_STATUS)
        hapi_severity = self.__get_parameter(msg, self.__severity_key,
                                             self.__default_severity,
                                             haplib.TRIGGER_SEVERITY)


        events = []
        events.append({
            "eventId": event_id,
            "time": hapcommon.conv_to_hapi_time(timestamp),
            "type": hapi_event_type,
            "status": hapi_status,
            "severity": hapi_severity,
            "hostId": host,
            "hostName": host,
            "brief": brief,
            "extendedInfo": ""
        })
        self.divide_and_put_data(self.put_events, events,
                           last_info_generator=lambda x: None)

    def __get_parameter(self, msg, key, default_value, candidates):
        param = msg.get(key, default_value)
        if param not in candidates:
            logger.error("Unknown parameter: %s for key: %s" % (param, key))
            param = default_value
        return param

    def __generate_event_id(self):
        return datetime.datetime.utcnow().strftime("%Y%m%d%H%M%S%f")

    def __parse_line(self, line):
        try:
            if not self.__re_expected_msg.search(line):
                logger.error("%% " + line.rstrip("\n"))
                return None, None, None
            header, msg = line.split(": ", 1)
            timestamp, tag = self.__parse_header(header)
        except:
            timestamp, tag, msg = None, None, None
            hap.handle_exception()
            self.__arm_info.fail()
        return timestamp, tag, msg

    def __parse_header(self, header):
        LEN_DATE_TIME = 19
        timestamp = datetime.datetime.strptime(header[:LEN_DATE_TIME],
                                               "%Y-%m-%d %H:%M:%S")
        IDX_OFS_SIGN = LEN_DATE_TIME + 1
        IDX_OFS_HOUR = IDX_OFS_SIGN + 1
        IDX_OFS_MINUTE = IDX_OFS_HOUR + 2
        offset_sign = header[IDX_OFS_SIGN]
        offset_hour = int(header[IDX_OFS_HOUR:IDX_OFS_HOUR+2])
        offset_hour *= {"+": 1, "-": -1}[offset_sign]
        offset_minute = int(header[IDX_OFS_MINUTE:IDX_OFS_MINUTE+2])

        offset = datetime.timedelta(hours=offset_hour, minutes=offset_minute)
        utc_timestamp = timestamp - offset

        IDX_HEADER_BEGIN = IDX_OFS_MINUTE + 3
        tag = header[IDX_HEADER_BEGIN:]
        return utc_timestamp, tag

class Hap2Fluentd(standardhap.StandardHap):

    def __init__(self):
        standardhap.StandardHap.__init__(self)

        parser = self.get_argument_parser()
        parser.add_argument("--fluentd-launch",
                            default="/usr/sbin/td-agent --suppress-config-dump",
                            help="A command line to launch fluentd.")
        parser.add_argument("--tag", default="^hatohol\..*",
                            help="A regular expression of the target tag.")

    def on_parsed_argument(self, args):
        self.__args = args

    def create_main_plugin(self, *args, **kwargs):
        plugin = Hap2FluentdMain(*args, **kwargs)
        plugin.set_arguments(self.__args)
        return plugin

if __name__ == '__main__':
    Hap2Fluentd().run()
