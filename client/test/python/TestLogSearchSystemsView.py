#!/usr/bin/env python
"""
  Copyright (C) 2014 Project Hatohol

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
from django.http import HttpRequest
import json
import httplib
import urllib
from hatohol.models import LogSearchSystem
from hatohol.views import log_search_systems
from hatohol_server_emulator import HatoholServerEmulator
from hatohol import hatoholserver


class TestLogSearchSystemsView(unittest.TestCase):

    def _setup_emulator(self):
        self._emulator = HatoholServerEmulator()
        self._emulator.start_and_wait_setup_done()

    def _request(self, method, id=None, body=None, POST=None):
        request = HttpRequest()
        request.method = method
        self._setSessionId(request)
        if body:
            request.META['CONTENT_TYPE'] = "application/x-www-form-urlencoded"
            request._body = urllib.urlencode(body)
        if POST:
            request.POST = POST
        return log_search_systems(request, id)

    def _get(self, id=None):
        return self._request('GET', id=id)

    def _post(self, body=None):
        return self._request('POST', POST=body)

    def _put(self, id=None, body=None):
        return self._request('PUT', id=id, body=body)

    def _delete(self, id=None):
        return self._request('DELETE', id=id)

    def setUp(self):
        LogSearchSystem.objects.all().delete()
        self._setup_emulator()

    def tearDown(self):
        if self._emulator is not None:
            self._emulator.shutdown()
            self._emulator.join()
            del self._emulator


class TestLogSearchSystemsViewAuthorized(TestLogSearchSystemsView):

    def _setSessionId(self, request):
        # The following session ID is just fake, because the request is
        # recieved in the above HatoholServerEmulatorHandler that
        # acutually doesn't verify it.
        request.META[hatoholserver.SESSION_NAME_META] = \
            'c579a3da-65db-44b4-a0da-ebf27548f4fd'

    def test_get_without_id(self):
        system = LogSearchSystem(
            type='groonga',
            base_url='http://search.example.com/#!/tables/Logs/search')
        system.save()
        response = self._get(None)
        self.assertEquals(response.status_code, httplib.OK)
        record = {
            'id': system.id,
            'type': system.type,
            'base_url': system.base_url,
        }
        self.assertEquals(json.loads(response.content),
                          [record])

    def test_get_with_id(self):
        system = LogSearchSystem(
            type='groonga',
            base_url='http://search.example.com/#!/tables/Logs/search')
        system.save()
        response = self._get(system.id)
        self.assertEquals(response.status_code, httplib.OK)
        record = {
            'id': system.id,
            'type': system.type,
            'base_url': system.base_url,
        }
        self.assertEquals(json.loads(response.content),
                          record)

    def test_get_with_id_nonexsitent(self):
        system = LogSearchSystem(
            type='groonga',
            base_url='http://search.example.com/#!/tables/Logs/search')
        system.save()
        id = system.id
        system.delete()
        response = self._get(id)
        self.assertEquals(response.status_code, httplib.NOT_FOUND)

    def test_post(self):
        record = {
            'type': 'groonga',
            'base_url': 'http://search.example.com/#!/tables/Logs/search',
        }
        response = self._post(record)
        self.assertEquals(response.status_code, httplib.CREATED)
        system = LogSearchSystem.objects.get(type=record['type'])
        record['id'] = system.id
        self.assertEquals(json.loads(response.content),
                          record)

    def test_put_without_id(self):
        record = {
            'type': 'groonga',
            'base_url': 'http://search.example.com/#!/tables/Logs/search',
        }
        response = self._put(None, record)
        self.assertEquals(response.status_code, httplib.BAD_REQUEST)

    def test_put_with_id(self):
        system = LogSearchSystem(
            type='groonga',
            base_url='http://search.example.com/#!/tables/Logs/search')
        system.save()
        new_record = {
            'type': 'other',
            'base_url': 'http://search.example.net/',
        }
        response = self._put(system.id, new_record)
        self.assertEquals(response.status_code, httplib.OK)
        record = {
            'id': system.id,
            'type': new_record['type'],
            'base_url': new_record['base_url'],
        }
        self.assertEquals(json.loads(response.content),
                          record)

    def test_put_with_id_nonexistent(self):
        system = LogSearchSystem(
            type='groonga',
            base_url='http://search.example.com/#!/tables/Logs/search')
        system.save()
        nonexistent_id = system.id
        system.delete()
        record = {
            'type': 'other',
            'base_url': 'http://search.example.net/',
        }
        response = self._put(nonexistent_id, record)
        self.assertEquals(response.status_code, httplib.NOT_FOUND)

    def test_delete_without_id(self):
        response = self._delete(None)
        self.assertEquals(response.status_code, httplib.BAD_REQUEST)

    def test_delete_with_id(self):
        system = LogSearchSystem(
            type='groonga',
            base_url='http://search.example.com/#!/tables/Logs/search')
        system.save()
        response = self._delete(system.id)
        self.assertEquals(response.status_code, httplib.OK)
        self.assertRaises(LogSearchSystem.DoesNotExist,
                          lambda: LogSearchSystem.objects.get(id=system.id))

    def test_delete_with_id_nonexistent(self):
        system = LogSearchSystem()
        system.save()
        nonexistent_id = system.id
        system.delete()
        response = self._delete(nonexistent_id)
        self.assertEquals(response.status_code, httplib.NOT_FOUND)


class TestLogSearchSystemsViewUnauthorized(TestLogSearchSystemsView):

    def _setSessionId(self, request):
        pass  # Don't set session ID

    def test_get_without_id(self):
        response = self._get(None)
        self.assertEquals(response.status_code, httplib.FORBIDDEN)
