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

from hatohol import voyager

class TestHatoholEscapeException:
  pass

class TestHatoholVoyager(unittest.TestCase):

  def test_show_server(self):

    ctx = {"url":None}
    def url_hook(hook_url):
      ctx["url"] = hook_url
      raise TestHatoholEscapeException()

    voyager.set_url_open_hook(url_hook)
    try:
      voyager.main(["show-server"])
    except TestHatoholEscapeException:
      pass
    self.assertEquals("http://localhost:33194/server", ctx["url"])

if __name__ == '__main__':
    unittest.main()
