"""
  Copyright (C) 2013 Project Hatohol

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

from BaseHTTPServer import BaseHTTPRequestHandler
from BaseHTTPServer import HTTPServer
import json
import urlparse
import threading
import httplib
from hatohol import hatoholserver
from hatohol import hatohol_def


class HatoholServerEmulationHandler(BaseHTTPRequestHandler):
    def _request_user_me(self):
        self.send_response(httplib.OK)
        body_dict = {'apiVersion': hatohol_def.FACE_REST_API_VERSION,
                     'errorCode': hatohol_def.HTERR_OK,
                     'numberOfUsers': 1,
                     'users': [{'userId': 5, 'name': 'hogetaro', 'flags': 0}]}
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
