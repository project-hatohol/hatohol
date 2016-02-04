#!/usr/bin/env python
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

import json
import unittest
import datetime
import testutils
from hatohol import haplib
from hatohol import hapcommon
import inspect

class HapCommon(unittest.TestCase):
    def test_translate_unix_time_to_hatohol_time(self):
        result = hapcommon.translate_unix_time_to_hatohol_time(0)
        self.assertEquals(result, "19700101000000.000000000")
        result = hapcommon.translate_unix_time_to_hatohol_time("0")
        self.assertEquals(result, "19700101000000.000000000")
        result = hapcommon.translate_unix_time_to_hatohol_time(0, 123456789)
        self.assertEquals(result, "19700101000000.123456789")
        result = hapcommon.translate_unix_time_to_hatohol_time(0, 123456)
        self.assertEquals(result, "19700101000000.000123456")
        result = hapcommon.translate_unix_time_to_hatohol_time("0", "123456789")
        self.assertEquals(result, "19700101000000.123456789")

        result = hapcommon.translate_unix_time_to_hatohol_time("0", "999999999")
        self.assertEquals(result, "19700101000000.999999999")

    def test_translate_unix_time_to_hatohol_time_with_errors(self):
        self.assertRaises(
            ValueError,
            hapcommon.translate_unix_time_to_hatohol_time, 0, -1)

        self.assertRaises(
            ValueError,
            hapcommon.translate_unix_time_to_hatohol_time, 0, 10000000000)

        self.assertRaises(
            ValueError,
            hapcommon.translate_unix_time_to_hatohol_time, 0, "")


    def test_translate_hatohol_time_to_unix_time(self):
        result = hapcommon.translate_hatohol_time_to_unix_time("19700101000000.123456789")
        # This result is utc time
        self.assertAlmostEquals(result, 0.123456789, delta=0.000000001)

    def test_get_biggest_num_of_dict_array(self):
        test_target_array = [{"test_value": 3},
                             {"test_value": 7},
                             {"test_value": 9}]
        result = hapcommon.get_biggest_num_of_dict_array(test_target_array,
                                                         "test_value")
        self.assertEquals(result, 9)

    def test_get_current_hatohol_time(self):
        result = hapcommon.get_current_hatohol_time()

        self.assertTrue(14 < len(result) < 22)
        ns = int(result[15: 21])
        self.assertTrue(0 <= ns < 1000000)

    def test_conv_to_hapi_time(self):
        dt = datetime.datetime(2015, 6, 28, 9, 35, 11, 123456)
        self.assertEquals(hapcommon.conv_to_hapi_time(dt),
                          "20150628093511.123456")

    def test_conv_to_hapi_time_with_offset(self):
        dt = datetime.datetime(2015, 6, 28, 9, 35, 11, 123456)
        ofs = -datetime.timedelta(hours=1, minutes=35, seconds=1)
        self.assertEquals(hapcommon.conv_to_hapi_time(dt, ofs),
                          "20150628080010.123456")

    def test_translate_int_to_decimal(self):
        test_nano_sec = 123456789
        expect_result = 0.123456789
        result = hapcommon.translate_int_to_decimal(test_nano_sec)

        self.assertEquals(result, expect_result)

    def test_get_top_file_name(self):
        expect_result = "runpy.pyc"
        self.assertEquals(hapcommon.get_top_file_name(), expect_result)
