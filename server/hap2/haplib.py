#! /usr/bin/env python
# coding: UTF-8
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

SERVER_PROCEDURES = {"exchangeProfile": True,
                     "getMonitoringServerInfo": True,
                     "getLastInfo": True,
                     "putItems": True,
                     "putHistory": True,
                     "putHosts": True,
                     "putHostGroups": True,
                     "putHostGroupMembership": True,
                     "updateTriggers": True,
                     "updateEvents": True,
                     "updateHostParent": True,
                     "putArmInfo": True}

PROCEDURES_DEFS = {
    "exchangeProfile": {
        "args": {
            "procedures": {"type": list(), "mandatory": True},
            "name": {"type": unicode(), "mandatory": True},
         }
    },
    "fetchItems": {
        "args": {
            "hostIds": {"type": list(), "mandatory": False},
            "fetchId": {"type": unicode(), "mandatory": True},
        }
    },
    "fetchHistory": {
        "args": {
          "hostId":{"type": unicode(), "mandatory": True},
          "itemId": {"type": unicode, "mandatory": True},
          "beginTime": {"type": unicode(), "mandatory": True},
          "endTime": {"type": unicode(), "mandatory": True},
          "fetchId": {"type": unicode(), "mandatory": True},
        }
    },
    "fetchTriggers": {
        "args": {
            "hostIds": {"type": list(), "mandatory": False},
            "fetchId": {"type": unicode(), "mandatory": True}
        }
    },
    "fetchEvents": {
        "args": {
            "lastInfo":{"type": unicode(), "mandatory": True},
            "count":{"type": int(), "mandatory": True},
            # TODO: validate: direction
            "direction": {"type": unicode(), "mandatory": True},
            "fetchId": {"type": unicode(), "mandatory": True}
        }
    },
    "notifyMonitoringServerInfo": {
        "notification": True,
        "args": {} # TODO: fill content
    },
    "getMonitoringServerInfo": {
        "args": {}
    },
    "putHosts": {
        "args": {
            "hosts": {"type": list(), "mandatory": True}
        }
    },
    "putHostGroups": {
        "args": {
            "hostGroups": {"type": list(), "mandatory": True}
        }
    },
    "putHostGroupMembership": {
        "args": {
            "hostGroupMembership": {"type": list(), "mandatory": True}
        }
    },
    "putTriggers": {
        "args": {
            "triggers": {"type": list(), "mandatory": True},
            "updateType": {"type": unicode(), "mandatory": True}
        }
    },
    "putEvents": {
        "args": {
            "events": {"type": list(), "mandatory": True}
        }
    },
    "getLastInfo": {
        "args": {}
    },
    "putArmInfo": {
        "args": {}
    },
}


ERR_CODE_INVALID_REQUEST = -32600
ERR_CODE_METHOD_NOT_FOUND = -32601
ERR_CODE_INVALID_PARAMS = -32602
ERR_CODE_PARSER_ERROR = -32700
ERROR_DICT = {
    ERR_CODE_INVALID_REQUEST: "invalid Request",
    ERR_CODE_METHOD_NOT_FOUND: "Method not found",
    ERR_CODE_INVALID_PARAMS: "invalid params",
    ERR_CODE_PARSER_ERROR: "Parse error",
}

MAX_EVENT_CHUNK_SIZE = 1000

