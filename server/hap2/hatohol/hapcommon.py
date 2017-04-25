#!/usr/bin/env python
# coding: UTF-8
#
#  Copyright (C) 2016 Project Hatohol
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

import time
import math
import inspect
import calendar
from datetime import datetime
from datetime import timedelta

def translate_unix_time_to_hatohol_time(unix_time, ns=0):
    """
    Translate unix_time into a time string of HAPI2.

    @param unix_time An unix time (integer or string).
    @param ns A nanosecnd part of the time (integer or string).
    @return A timestamp string in HAPI2
    """
    utc_time = time.gmtime(int(unix_time))
    hatohol_time = time.strftime("%Y%m%d%H%M%S", utc_time)
    ns_int = int(ns)
    if ns_int > 1000000000 or ns_int < 0:
        raise ValueError("Invalid 'ns': %s" % ns)
    return hatohol_time + ".%09d" % ns_int

def translate_hatohol_time_to_unix_time(hatohol_time):
    ns = int()
    if "." in hatohol_time:
        hatohol_time, ns = hatohol_time.split(".")
    date_time = datetime.strptime(hatohol_time, "%Y%m%d%H%M%S")
    unix_time =  calendar.timegm(date_time.timetuple())
    return unix_time + int(ns) / float(10 ** 9)

def get_biggest_num_of_dict_array(array, key):
    last_info = None

    digit = int()
    for target_dict in array:
        if isinstance(target_dict[key], int):
            break
        if digit < len(target_dict[key]):
            digit = len(target_dict[key])

    for target_dict in array:
        target_value = target_dict[key]
        if isinstance(target_value, str) or isinstance(target_value, unicode):
            target_value = target_value.zfill(digit)

        if last_info < target_value:
            last_info = target_value

    return last_info

def get_current_hatohol_time():
    utc_now = datetime.utcnow()
    return utc_now.strftime("%Y%m%d%H%M%S.") + str(utc_now.microsecond)

def conv_to_hapi_time(date_time, offset=timedelta()):
    """
    Convert a datetime object to a string formated for HAPI2
    @param date_time A datatime object
    @param offset    An offset to the time
    @return A string of the date and time in HAPI2
    """
    adjust_time = date_time + offset
    return adjust_time.strftime("%Y%m%d%H%M%S.") \
                + "%06d" % adjust_time.microsecond

def translate_int_to_decimal(nano_sec):
    return float(nano_sec) / 10 ** (int(math.log10(nano_sec)) + 1)

def get_top_file_name():
    if not top_frame.f_globals.get("__file__"):
        return "Interactive"

    current_frame = inspect.currentframe()
    while True:
        if current_frame is None:
            break
        top_frame = current_frame
        current_frame = top_frame.f_back

    try:
        top_file_name = top_frame.f_globals["__file__"]
    except KeyError:
        return "Interactive"

    slash_index = top_file_name.rfind("/")
    if slash_index:
        top_file_name = top_file_name[slash_index+1:]

    return top_file_name

def insert_divide_info_to_params(params, serial_id, is_last, request_id):
    params["divideInfo"] = dict()
    params["divideInfo"]["serialId"] = serial_id
    params["divideInfo"]["isLast"] = is_last
    params["divideInfo"]["requestId"] = request_id
