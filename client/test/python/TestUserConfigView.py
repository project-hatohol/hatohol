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
from django.http import QueryDict
from BaseHTTPServer import BaseHTTPRequestHandler
from BaseHTTPServer import HTTPServer
import urlparse
import threading
import json
import httplib
from viewer import userconfig
from hatohol import hatoholserver
from hatohol import hatohol_def

class HatoholServerEmulationHandler(BaseHTTPRequestHandler):
    def _request_user_me(self):
        self.send_response(httplib.OK)
        body_dict = {'apiVersion': hatohol_def.FACE_REST_API_VERSION,
                     'errorCode': hatohol_def.HTERR_OK,
                     'numberOfUsers': 1,
                     'users': [{'userId':5, 'name':'hogetaro', 'flags':0}]}
        return json.dumps(body_dict)

    def do_GET(self):
        parsed_path = urlparse.urlparse(self.path)
        body = ""
        if parsed_path.path == '/user/me':
            body = self._request_user_me()
        else:
            self.send_response(httplib.NOT_FOUND)
        self.end_headers()
        self.wfile.write(body)


class EmulationHandlerNotReturnUserInfo(HatoholServerEmulationHandler):
    def _request_user_me(self):
        self.send_response(httplib.OK)
        body_dict = {'apiVersion': hatohol_def.FACE_REST_API_VERSION,
                     'errorCode': hatohol_def.HTERR_OK,
                     'numberOfUsers': 0,
                     'users': []}
        return json.dumps(body_dict)

class HatoholServerEmulator(threading.Thread):

    def __init__(self, handler=HatoholServerEmulationHandler):
        threading.Thread.__init__(self)
        self._server = None
        self._setup_done_evt = threading.Event()
        self._emulation_handler = handler

    def run(self):
        addr = hatoholserver.get_address()
        port = hatoholserver.get_port()
        self._server = HTTPServer((addr, port), self._emulation_handler)
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

    def _setSessionId(self, request):
        # The following session ID is just fake, because the request is
        # recieved in the above HatoholServerEmulatorHandler that
        # acutually doesn't verify it.
        request.META[hatoholserver.SESSION_NAME_META] = \
          'c579a3da-65db-44b4-a0da-ebf27548f4fd';

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
        request = HttpRequest()
        request.GET = QueryDict('items[]=foo.goo')
        self._setSessionId(request)
        response = userconfig.index(request)
        self.assertEquals(response.status_code, httplib.OK)
        items = json.loads(response.content)
        self.assertEquals(items['foo.goo'], None)

    def test_index_without_session_id(self):
        self._emulator = HatoholServerEmulator()
        self._emulator.start_and_wait_setup_done()
        request = HttpRequest()
        request.GET = QueryDict('items[]=foo')
        response = userconfig.index(request)
        self.assertEquals(response.status_code, httplib.BAD_REQUEST)

    def test_index_server_not_return_userinfo(self):
        self._emulator = HatoholServerEmulator(handler=EmulationHandlerNotReturnUserInfo)
        self._emulator.start_and_wait_setup_done()
        request = HttpRequest()
        self._setSessionId(request)
        request.GET = QueryDict('items[]=foo-item')
        response = userconfig.index(request)
        self.assertEquals(response.status_code, httplib.INTERNAL_SERVER_ERROR)

    def test_store(self):
        self._emulator = HatoholServerEmulator()
        self._emulator.start_and_wait_setup_done()
        request = HttpRequest()
        request.method = "POST"
        request.POST = QueryDict('favorite=dog')
        self._setSessionId(request)
        response = userconfig.index(request)
        self.assertEquals(response.status_code, httplib.OK)

    def test_store_and_get(self):
        self.test_store()
        request = HttpRequest()
        request.GET = QueryDict('items[]=favorite')
        self._setSessionId(request)
        response = userconfig.index(request)
        self.assertEquals(response.status_code, httplib.OK)
        items = json.loads(response.content)
        self.assertEquals(items['favorite'], 'dog')

