#!/usr/bin/env python
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
from django.http import HttpRequest
import json
import httplib
import urllib
from hatohol.models import Graph
from hatohol.views import graphs
from hatohol_server_emulator import HatoholServerEmulator
from hatohol import hatoholserver


class TestGraphsView(unittest.TestCase):

    def _setup_emulator(self):
        self._emulator = HatoholServerEmulator()
        self._emulator.start_and_wait_setup_done()

    def _request(self, method, id=None, body=None, POST=None, PUT=None):
        request = HttpRequest()
        request.method = method
        self._setSessionId(request)
        if PUT:
            request.META['CONTENT_TYPE'] = "application/json"
            request._body = PUT
        elif POST:
            request.META['CONTENT_TYPE'] = "application/json"
            request._body = POST
        elif body:
            request.META['CONTENT_TYPE'] = "application/x-www-form-urlencoded"
            request._body = urllib.urlencode(body)
        return graphs(request, id)

    def _get(self, id=None):
        return self._request('GET', id=id)

    def _post(self, body=None):
        return self._request('POST', POST=body)

    def _put(self, id=None, body=None):
        return self._request('PUT', id=id, PUT=body)

    def _delete(self, id=None):
        return self._request('DELETE', id=id)

    def setUp(self):
        Graph.objects.all().delete()
        self._setup_emulator()

    def tearDown(self):
        if self._emulator is not None:
            self._emulator.shutdown()
            self._emulator.join()
            del self._emulator


class TestGraphsViewAuthorized(TestGraphsView):

    def _setSessionId(self, request):
        # The following session ID is just fake, because the request is
        # recieved in the above HatoholServerEmulatorHandler that
        # acutually doesn't verify it.
        request.META[hatoholserver.SESSION_NAME_META] = \
            'c579a3da-65db-44b4-a0da-ebf27548f4fd'

    def test_get(self):
        graph = Graph(
            user_id=5,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        response = self._get(None)
        self.assertEquals(response.status_code, httplib.OK)
        record = {
            'id': graph.id,
            'user_id': graph.user_id,
            'server_id': 1,
            'host_id': 2,
            'item_id': 3,
        }
        self.assertEquals(json.loads(response.content),
                          [record])

    def test_get_without_own_record(self):
        graph = Graph(
            user_id=4,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        response = self._get(None)
        self.assertEquals(response.status_code, httplib.OK)
        self.assertEquals(json.loads(response.content),
                          [])

    def test_get_with_id(self):
        graph = Graph(
            user_id=5,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        response = self._get(graph.id)
        self.assertEquals(response.status_code, httplib.OK)
        record = {
            'id': graph.id,
            'user_id': graph.user_id,
            'server_id': 1,
            'host_id': 2,
            'item_id': 3
        }
        self.assertEquals(json.loads(response.content),
                          record)

    def test_get_graph_with_id_nonowner(self):
        graph = Graph(
            user_id=4,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        response = self._get(graph.id)
        self.assertEquals(response.status_code, httplib.FORBIDDEN)

    def test_get_with_id_nonexsitent(self):
        graph = Graph(
            user_id=5,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        id = graph.id
        graph.delete()
        response = self._get(id)
        self.assertEquals(response.status_code, httplib.NOT_FOUND)

    def test_post(self):
        user_id = 5
        record = {
            'server_id': 1,
            'host_id': 2,
            'item_id': 3,
        }
        response = self._post(json.dumps(record))
        self.assertEquals(response.status_code, httplib.CREATED)
        graph = Graph.objects.get(user_id=user_id)
        record['id'] = graph.id
        record['user_id'] = user_id
        self.assertEquals(json.loads(response.content),
                          record)

    def test_post_broken_json(self):
        broken_json = '{server_id:1,host_id:2,item_id:3}'
        response = self._post(broken_json)
        self.assertEquals(response.status_code, httplib.BAD_REQUEST)

    def test_put_without_id(self):
        record = {
            'server_id': 1,
            'host_id': 2,
            'item_id': 3,
        }
        response = self._put(None, json.dumps(record))
        self.assertEquals(response.status_code, httplib.BAD_REQUEST)

    def test_put_with_id(self):
        graph = Graph(
            user_id=5,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        new_record = {
            'server_id': 4,
            'host_id': 5,
            'item_id': 6,
        }
        settings_json = json.dumps(new_record)
        response = self._put(graph.id, settings_json)
        self.assertEquals(response.status_code, httplib.OK)
        record = {
            'id': graph.id,
            'user_id': graph.user_id,
        }
        record.update(new_record)
        self.assertEquals(json.loads(response.content),
                          record)

    def test_put_with_id_nonowner(self):
        graph = Graph(
            user_id=4,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        new_record = {
            'server_id': 4,
            'host_id': 5,
            'item_id': 6,
        }
        settings_json = json.dumps(new_record)
        response = self._put(graph.id, settings_json)
        self.assertEquals(response.status_code, httplib.FORBIDDEN)
        graph_in_db = Graph.objects.get(id=graph.id)
        self.assertEquals(graph, graph_in_db)

    def test_put_with_id_nonexistent(self):
        graph = Graph(
            user_id=5,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        nonexistent_id = graph.id
        graph.delete()
        record = {
            'server_id': 4,
            'host_id': 5,
            'item_id': 6,
        }
        response = self._put(nonexistent_id, record)
        self.assertEquals(response.status_code, httplib.NOT_FOUND)

    def test_delete_without_id(self):
        response = self._delete(None)
        self.assertEquals(response.status_code, httplib.BAD_REQUEST)

    def test_delete_with_id(self):
        graph = Graph(
            user_id=5,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        response = self._delete(graph.id)
        self.assertEquals(response.status_code, httplib.OK)
        self.assertRaises(Graph.DoesNotExist,
                          lambda: Graph.objects.get(id=graph.id))

    def test_delete_with_id_nonowner(self):
        graph = Graph(
            user_id=4,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        response = self._delete(graph.id)
        self.assertEquals(response.status_code, httplib.FORBIDDEN)

    def test_delete_with_id_nonexistent(self):
        graph = Graph(
            user_id=5
        )
        graph.save()
        nonexistent_id = graph.id
        graph.delete()
        response = self._delete(nonexistent_id)
        self.assertEquals(response.status_code, httplib.NOT_FOUND)


class TestGraphsViewUnauthorized(TestGraphsView):

    def _setSessionId(self, request):
        pass  # Don't set session ID

    def test_get(self):
        response = self._get(None)
        self.assertEquals(response.status_code, httplib.FORBIDDEN)
