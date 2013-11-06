#!/usr/bin/env python
"""
  Copyright (C) 2013 Project Hatohol

  This file is part of Hatohol.

  Hatohol is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  Hatohol is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
"""
import unittest

from hatohol.models import UserConfig

class TestUserConfig(unittest.TestCase):

    def test_create_integer(self):
        user_conf = UserConfig(item_name="age", user_id=5, value=17)
        self.assertEquals(user_conf.item_name, "age");
        self.assertEquals(user_conf.user_id, 5);
        self.assertEquals(user_conf.value, 17);

    def test_create_string(self):
        user_conf = UserConfig(item_name="name", user_id=5, value="Foo")
        self.assertEquals(user_conf.item_name, "name");
        self.assertEquals(user_conf.user_id, 5);
        self.assertEquals(user_conf.value, "Foo");

    def test_create_float(self):
        user_conf = UserConfig(item_name="height", user_id=5, value=172.5)
        self.assertEquals(user_conf.item_name, "height");
        self.assertEquals(user_conf.user_id, 5);
        self.assertEquals(user_conf.value, 172.5);


if __name__ == '__main__':
    unittest.main()
