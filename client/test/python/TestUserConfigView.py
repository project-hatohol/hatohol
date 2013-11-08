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
from django.http import HttpRequest
from BaseHTTPServer import BaseHTTPRequestHandler
from BaseHTTPServer import HTTPServer
import urlparse
import threading
import json
import httplib
from viewer import userconfig
from hatohol import hatoholserver

class HatoholServerEmulationHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed_path = urlparse.urlparse(self.path)
        body = ""
        if parsed_path.path == '/user/me':
            body = json.dumps({'userId':5, 'name':'hogetaro', 'flags':0})
            self.send_response(httplib.OK)
        else:
            self.send_response(httplib.NOT_FOUND)
        self.end_headers()
        self.wfile.write(body)

class HatoholServerEmulator(threading.Thread):

    def __init__(self):
        threading.Thread.__init__(self)
        self._server = None
        self._setup_done_evt = threading.Event()

    def run(self):
        addr = hatoholserver.get_address()
        port = hatoholserver.get_port()
        self._server = HTTPServer((addr, port), HatoholServerEmulationHandler)
        self._setup_done_evt.set()
        self._server.serve_forever()

    def start_and_wait_setup_done(self):
        self.start()
        self._setup_done_evt.wait()

    def shutdown(self):
        if self._server is None:
            return
        self._server.shutdown()
        del self._server

class TestUserConfigView(unittest.TestCase):

    #
    # Test cases
    #
    def tearDown(self):
        if self._emulator is not None:
            self._emulator.shutdown()
            self._emulator.join()
            del self._emulator

    def test_index(self):
        self._emulator = HatoholServerEmulator()
        self._emulator.start_and_wait_setup_done()
        item_name = "foo.goo"
        request = HttpRequest()
        # The followiing session ID is acutually not verified. So the value
        # is just fake.
        request.META[hatoholserver.SESSION_NAME_META] = 'c579a3da-65db-44b4-a0da-ebf27548f4fd';
        response = userconfig.index(request, item_name)
        self.assertEquals(response.status_code, httplib.OK)

    def test_index_without_session_id(self):
        self._emulator = HatoholServerEmulator()
        self._emulator.start_and_wait_setup_done()
        response = userconfig.index(HttpRequest(), "foo")
        self.assertEquals(response.status_code, httplib.BAD_REQUEST)
