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
import haplib
import time

class Gadget:
    def __init__(self):
        self.num_called = 0

    def __call__(self, arg1, arg2=None, arg3=None, arg4=None):
        self.args = (arg1, arg2, arg3, arg4)
        self.num_called += 1

class TestHaplib_handle_exception(unittest.TestCase):

    def test_handle_exception(self):
        obj = Gadget()
        try:
            raise obj
        except:
            exctype, value = haplib.handle_exception()
        self.assertEquals(Gadget, exctype)
        self.assertEquals(obj, value)

    def test_handle_exception_on_raises(self):
        try:
            raise TypeError
        except:
            self.assertRaises(TypeError, haplib.handle_exception, (TypeError,))

class TestHaplib_Signal(unittest.TestCase):

    def test_default(self):
        obj = haplib.Signal()
        self.assertEquals(False, obj.restart)

    def test_restart_is_true(self):
        obj = haplib.Signal(restart=True)
        self.assertEquals(True, obj.restart)


class TestHaplib_Callback(unittest.TestCase):

    def test_register_and_call(self):
        cb = haplib.Callback()
        handler = Gadget()
        cb.register(1, handler)
        arg1 = "a"
        arg2 = None
        arg3 = 1.3
        arg4 = True
        command_code = 1
        cb(command_code, arg1, arg2, arg3=arg3, arg4=arg4)
        self.assertEquals((arg1, arg2, arg3, arg4), handler.args)

    def test_call_with_no_handlers(self):
        cb = haplib.Callback()
        command_code = 1
        cb(command_code)

class CommandQueue(unittest.TestCase):

    def test_push_and_wait(self):
        code = 2
        args = ('a', 2, -1.5)
        cq = haplib.CommandQueue()
        gadz = Gadget()
        cq.register(code, gadz)
        cq.push(code, args)
        # This is not smart. The processing won't be completed within 'duration'
        duration = 0.1
        cq.wait(duration)
        self.assertEquals((args, None, None, None), gadz.args)

    def test_pop_all(self):
        code = 3
        num_push = 5
        args = ('a', 2, -1.5)
        cq = haplib.CommandQueue()
        gadz = Gadget()
        cq.register(code, gadz)
        self.assertEquals(0, gadz.num_called)
        [cq.push(code, args) for i in range(0, num_push)]
        # Too ugly. But we don't have other way to wait for completion of
        # processing in the background thread.
        while cq.__q.empty():
            time.sleep(0.01)
        cq.pop_all()
        self.assertEquals(num_push, gadz.num_called)
