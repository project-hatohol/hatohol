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
 
# The document for making custom field is
# https://docs.djangoproject.com/en/dev/howto/custom-model-fields/

class SmartField(models.Field):

    description = "A field that can store any type"

    __metaclass__ = models.SubfieldBase

    def db_type(self, connection):
        # We only support MySQL at present.
        if connection.settings_dict['ENGINE'] == 'django.db.backends.mysql':
            return 'LONGBLOB'
        else:
            raise NotImplementedError

    def to_python(self, value):
        if isinstance(value, str):
            try:
                return cPickle.loads(value)        
            except:
                pass
        return value

    def get_db_prep_save(self, value):
        return cPickle.dumps(value, cPickle.HIGHEST_PROTOCOL)
