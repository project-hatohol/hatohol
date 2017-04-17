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


import json

RESULTS = {
    "user.authenticate": "test_auth_token",
    "apiinfo.version": "2.2.7",
    "item.get": [{"itemid": "1", "hostid": "1",
        "name": "test_name","units": "B","value_type":"3",
        "applications": [{"applicationid": "test_id", "name": "test_name"}],
        "lastclock": "0", "lastns":"111111111", "lastvalue": "100"}],
    "history.get": [{"itemid": "1", "clock": "0", "value": "1", "ns": "111111111"},
                    {"itemid": "1", "clock": "1", "value": "2", "ns": "222222222"}],
    "host.get": [{"hostid": "1", "name":"test_host", "groups":[{"groupid":"1"}]}],
    "hostgroup.get": [{"groupid":"1", "name":"test_name"}],
    "trigger.get": [{"triggerid":"1", "priority":3,
                     "description":"test_description", "lastchange":"0",
                     "hosts":[{"hostid":"1","name":"test_host"}], "state":"0"}],
    "trigger.get.expand":[{"triggerid":"1", "priority": 3,
                           "description":"test_expand_description",
                           "state": "0"}],
    "event.get": [{"eventid":"1", "objectid":"1",
                   "clock":"0","value":"0", "ns":"111111111",
                   "hosts":[{"hostid":"1","name":"test_host"}]}]
}

class urllib2:
    def Request(self, url, post, header):
        request = json.loads(post)
        self.method = request["method"]
        if str(request["method"]) == "trigger.get":
            if request["params"].get(u"expandDescription"):
                self.method += ".expand"
        return

    def urlopen(self, request):
        response = Response(self.method)
        return response


class Response:

    def __init__(self, method):
        self.response = json.dumps({"result": RESULTS[method]})

    def read(self):
        return self.response
