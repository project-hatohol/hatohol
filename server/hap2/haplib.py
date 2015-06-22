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

import sys
import time
import logging
import traceback
import multiprocessing
import Queue

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

def handle_exception(raises=()):
    """
    Logging exception information including back trace and return
    some information. This method is supposed to be used in 'except:' block.
    Note that if the exception class is Signal, this method doesn't log it.

    @raises
    A sequence of exceptionclass names. If the handling exception is one of
    it, this method just raises it again.

    @return
    A sequence of exception class and the instance of the handling exception.
    """
    (exctype, value, tb) = sys.exc_info()
    if exctype in raises:
        raise
    if exctype is not Signal:
        logging.error("Unexpected error: %s, %s, %s" % \
                      (exctype, value, traceback.format_tb(tb)))
    return exctype, value


class Signal:
    """
    This class is supposed to raise as an exception in order to
    propagate some events and jump over stack frames.
    """

    def __init__(self, restart=False):
        self.restart = restart


class Callback:
    def __init__(self):
        self.__handlers = {}

    def register(self, code, handler):
        handler_list = self.__handlers.get(code)
        if handler_list is None:
            handler_list = []
            self.__handlers[code] = handler_list
        handler_list.append(handler)

    def __call__(self, code, *args, **kwargs):
        handler_list = self.__handlers.get(code)
        if handler_list is None:
            return
        for handler in handler_list:
            handler(*args, **kwargs)


class CommandQueue(Callback):
    def __init__(self):
        Callback.__init__(self)
        self.__q = multiprocessing.Queue()

    def wait(self, duration):
        """
        Wait for commands and run the command when it receives in the
        given duration.
        @parameter duration
        This method returns after the time of this parameter goes by.
        """
        wakeup_time = time.time() + duration
        while  True:
            sleep_time = wakeup_time - time.time()
            if sleep_time <= 0:
                return
            self.__wait(sleep_time)

    def __wait(self, sleep_time):
        try:
            code, args = \
                self.__q.get(block=True, timeout=sleep_time)
            self(code, args)
        except Queue.Empty:
            # If no command is forthcomming within timeout,
            # this path is executed.
            pass

    def push(self, code, args):
        self.__q.put((code, args), block=False)

    def pop_all(self):
        while not self.__q.empty():
            self.__wait(0)
