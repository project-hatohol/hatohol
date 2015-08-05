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

import unittest
import hap

class Gadget:
    pass

class handle_exception(unittest.TestCase):
    def test_handle_exception(self):
        obj = Gadget()
        try:
            raise obj
        except:
            exctype, value = hap.handle_exception()
        self.assertEquals(Gadget, exctype)
        self.assertEquals(obj, value)

    def test_handle_exception_on_raises(self):
        try:
            raise TypeError
        except:
            self.assertRaises(TypeError, hap.handle_exception, (TypeError,))


class Signal(unittest.TestCase):
    def test_default(self):
        obj = hap.Signal()
        self.assertEquals(False, obj.restart)
        self.assertEquals(False, obj.critical)

    def test_restart_is_true(self):
        obj = hap.Signal(restart=True)
        self.assertEquals(True, obj.restart)

    def test_critical_is_true(self):
        obj = hap.Signal(critical=True)
        self.assertEquals(True, obj.critical)


