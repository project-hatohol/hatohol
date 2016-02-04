#!/usr/bin/env python
"""
  Copyright (C) 2016 Project Hatohol

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
import os
import argparse
import unittest
import testutils
from collections import namedtuple
from hatohol import transporter
from hatohol.transporter import Transporter

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

class Manager(unittest.TestCase):
    def test_define_arguments(self):
        test_parser = argparse.ArgumentParser()
        manager = transporter.Manager(None)
        testutils.assertNotRaises(manager.define_arguments, test_parser)

    def test_register_and_find(self):
        class testTrans:
            @classmethod
            def define_arguments(cls, parser):
                pass

        manager = transporter.Manager(None)
        self.assertIsNone(manager.find("testTrans"))
        transporter.Manager.register(testTrans)
        self.assertEquals(manager.find("testTrans"), testTrans)

    def test_import_module_for_nonexisting_module(self):
        path = os.path.dirname(__file__)
        self.assertIsNone(transporter.Manager.import_module("notfound", path))

    def test_import_module(self):
        path = os.path.dirname(__file__)
        mod = transporter.Manager.import_module("stub", path)
        self.assertEquals(mod.a, 1)
