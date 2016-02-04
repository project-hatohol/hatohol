#!/usr/bin/env python
# coding: UTF-8
"""
  Copyright (C) 2015-2016 Project Hatohol

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
import hap
import Queue
import signal
import errno
import time

class Gadget:
    pass

class handle_exception(unittest.TestCase):
    def test_handle_exception(self):
        obj = Gadget()
        try:
            raise obj
        except:
            exctype, value = hap.handle_exception()
        self.assertEquals(Gadget, exctype)
        self.assertEquals(obj, value)

    def test_handle_exception_on_raises(self):
        try:
            raise TypeError
        except:
            self.assertRaises(TypeError, hap.handle_exception, (TypeError,))

    def test_handle_exception_critical_signal(self):
        try:
            raise hap.Signal(critical=True)
        except:
            self.assertRaises(hap.Signal, hap.handle_exception)


class Signal(unittest.TestCase):
    def test_default(self):
        obj = hap.Signal()
        self.assertEquals(False, obj.restart)
        self.assertEquals(False, obj.critical)

    def test_restart_is_true(self):
        obj = hap.Signal(restart=True)
        self.assertEquals(True, obj.restart)

    def test_critical_is_true(self):
        obj = hap.Signal(critical=True)
        self.assertEquals(True, obj.critical)


class MultiprocessingQueue(unittest.TestCase):
    def test_get_empty_in_nonblocking(self):
        q = hap.MultiprocessingQueue()
        self.assertRaises(Queue.Empty, q.get, block=False)

    def test_get_empty_in_blocking(self):
        q = hap.MultiprocessingQueue()
        self.assertRaises(Queue.Empty, q.get, timeout=0.1)

    def test_get_item(self):
        q = hap.MultiprocessingQueue()
        q.put(4)
        self.assertEquals(q.get(timeout=1), 4)

    def test_handle_ioerr_not_eintr(self):
        try:
            raise IOError(errno.ENOENT, "")
        except IOError as e:
            self.assertRaises(IOError, hap._handle_ioerr, e, None, None)

    def test_handle_ioerr_errrno_is_not_EINTR(self):
        try:
            raise IOError(errno.ENOENT, "")
        except IOError as e:
            self.assertRaises(IOError, hap._handle_ioerr, e, None, None)

    def test_handle_ioerr_timeout_is_None(self):
        e = IOError(errno.EINTR, "")
        self.assertIsNone(hap._handle_ioerr(e, None, None))

    def test_handle_ioerr_curr_time_over_expired_time(self):
        e = IOError(errno.EINTR, "")
        timeout = 5 # any positive integer is OK
        expired_time = time.time() - 1
        self.assertEquals(hap._handle_ioerr(e, timeout, expired_time), 0)

    def test_handle_ioerr_curr_not_expired(self):
        e = IOError(errno.EINTR, "")
        timeout = 5 # any positive integer is OK
        expired_time = time.time() + 10
        returned_timeout = hap._handle_ioerr(e, timeout, expired_time)
        self.assertGreater(returned_timeout, 0)
        self.assertLess(hap._handle_ioerr(e, timeout, expired_time), 10)
