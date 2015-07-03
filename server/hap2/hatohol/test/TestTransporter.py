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
from transporter import Transporter
import transporter

class TestTransporter(unittest.TestCase):
    def test_factory(self):
        obj = transporter.Factory.create(self.__default_transporter_args())
        self.assertEquals(obj.__class__.__name__, "Transporter")

    def test_setup_should_be_called(self):
        class TestTransporter:
            def setup(self, transporter_args):
                transporter_args["setup_called"] = True

        transporter_args = {"class": TestTransporter}
        obj = transporter.Factory.create(transporter_args)
        self.assertIsInstance(obj, TestTransporter)
        self.assertEquals(transporter_args.get("setup_called"), True)

    def test_get_receiver(self):
        tx = transporter.Factory.create(self.__default_transporter_args())
        self.assertIsNone(tx.get_receiver())

    def test_set_receiver(self):
        def receiver():
            pass

        def receiver2():
            pass

        tx = transporter.Factory.create(self.__default_transporter_args())
        tx.set_receiver(receiver)
        self.assertEquals(tx.get_receiver(), receiver)

        # update
        tx.set_receiver(receiver2)
        self.assertEquals(tx.get_receiver(), receiver2)

    def __default_transporter_args(self):
        return {"class": Transporter}
