#!/usr/bin/env python
"""
  Copyright (C) 2013 Project Hatohol

  This file is part of Hatohol.

  Hatohol is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  Hatohol is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
"""
import unittest

from hatohol import utils

class TestHatoholUtils(unittest.TestCase):

  def test_get_element(self):
    self.obj = {"foo":1, "bar":-5, "x":9};
    self.assertEquals(utils.get_element(self.obj, "bar"), self.obj["bar"])

  def test_get_element_not_found(self):
    self.obj = {"foo":1, "bar":-5, "x":9};
    self.assertEquals(utils.get_element(self.obj, "abc"), "")

  def test_get_element_fallback(self):
    self.obj = {"foo":1, "bar":-5, "x":9};
    self.assertEquals(utils.get_element(self.obj, "abc", "dog"), "dog")

if __name__ == '__main__':
    unittest.main()
