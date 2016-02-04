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

import unittest
from hatohol import transporters

class Init(unittest.TestCase):
    def test_is_valid_module_name_ends_with_py(self):
        self.assertTrue(transporters.is_valid_module_name("foo.py"))

    def test_is_valid_module_name_end_is_not_py(self):
        self.assertFalse(transporters.is_valid_module_name("foo.pi"))

    def test_is_valid_module_name_starts_with_underscore(self):
        self.assertFalse(transporters.is_valid_module_name("_foo.py"))

    def test_is_valid_module_name_start_is_not_underscore(self):
        self.assertTrue(transporters.is_valid_module_name("foo.py"))
