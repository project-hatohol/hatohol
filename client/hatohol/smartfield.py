# Copyright (C) 2013 Project Hatohol
#
# This file is part of Hatohol.
#
# Hatohol is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Hatohol is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Hatohol. If not, see <http://www.gnu.org/licenses/>.

from django.db import models
from django.conf import settings
import cPickle
import base64
 
# The document for making custom field is
# https://docs.djangoproject.com/en/dev/howto/custom-model-fields/

class SmartField(models.Field):

    description = "A field that can store any type"

    __metaclass__ = models.SubfieldBase

    # We have data as a object of UserConfigValue. The purpose is to
    # distinguish a string (that is encoded by Base64 + pickled) as a
    # configuration value with a string from database.
    class UserConfigValue:
        def __init__(self, value):
            self._value = value

        def get(self):
            return self._value

    def db_type(self, connection):
        # We only support MySQL at present.
        if connection.settings_dict['ENGINE'] == 'django.db.backends.mysql':
            return 'LONGBLOB'
        else:
            raise NotImplementedError

    def to_python(self, value):
        if isinstance(value, self.UserConfigValue):
            return value.get()

        # We assume this path is used when the data is read from the database.
        assert isinstance(value, str)
        pickle_data = base64.b64decode(value)
        return cPickle.loads(pickle_data)

    def get_db_prep_save(self, value, connection):
        # We use ASCII (Base64) pickle.  There are two reasons to use ASCII.
        # (1) Easy to see the content when we check it with mysql command.
        # (2) CursorDebugWrapper, which logs SQL statements when
        #     settings.DEBUG = True, cannot handle the binary string.
        return base64.b64encode(cPickle.dumps(value, cPickle.HIGHEST_PROTOCOL))
