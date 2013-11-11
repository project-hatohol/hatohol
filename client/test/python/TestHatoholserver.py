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
import os

from hatohol import hatoholserver

class TestHatoholserver(unittest.TestCase):

    def setUp(self):
        server_addr = os.getenv('HATOHOL_SERVER_ADDR')
        if server_addr:
            self._orig_server_addr = server_addr
        else:
            self._orig_server_addr = None

        server_port = os.getenv('HATOHOL_SERVER_PORT')
        if server_port:
            self._orig_server_port = server_port
        else:
            self._orig_server_port = None

    def tearDown(self):
        if self._orig_server_addr:
            os.environ['HATOHOL_SERVER_ADDR'] = self._orig_server_addr
            self._orig_server_addr = None
        elif 'HATOHOL_SERVER_ADDR' in os.environ:
            del os.environ['HATOHOL_SERVER_ADDR']

        if self._orig_server_port:
            os.environ['HATOHOL_SERVER_PORT'] = self._orig_server_port
            self._orig_server_port = None
        elif 'HATOHOL_SERVER_PORT' in os.environ:
            del os.environ['HATOHOL_SERVER_PORT']
        hatoholserver._setup() # Update the internal information

    def test_get_address(self):
        addr = self.assertEqual(hatoholserver.get_address(), 'localhost')

    def test_get_address_with_env(self):
        os.environ['HATOHOL_SERVER_ADDR'] = 'foo.example.com'
        hatoholserver._setup() # Update the internal information
        addr = self.assertEqual(hatoholserver.get_address(), 'foo.example.com')

    def test_get_default_port(self):
        del os.environ['HATOHOL_SERVER_PORT']
        hatoholserver._setup() # Update the internal information
        addr = self.assertEqual(hatoholserver.get_port(),
                                hatoholserver.DEFAULT_SERVER_PORT)

