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
from django.db import transaction
import smartfield

class UserConfig(models.Model):
    item_name = models.CharField(max_length=255, db_index=True)
    user_id = models.IntegerField(db_index=True)
    value = smartfield.SmartField()

    def __init__(self, *args, **kwargs):
        if 'value' in kwargs:
            value = kwargs['value'];
            kwargs['value'] = smartfield.SmartField.UserConfigValue(value)
        models.Model.__init__(self, *args, **kwargs)

    def __unicode__(self):
        return '%s (%d)' % (self.item_name, self.user_id)

    @classmethod
    def get_object(cls, item_name, user_id):
        """Get an object of UserConfig

        Args:
            item_name: A configuration item
            user_id: A user ID for the configuration

        Returns:
            If the matched item exists, it is returned. Otherwise, None is
            returned.
        """
        objs = UserConfig.objects.filter(item_name=item_name).filter(user_id=user_id)
        if not objs:
            return None
        assert len(objs) == 1
        return objs[0]

    @classmethod
    def get(cls, item_name, user_id):
        """Get a user configuration

        Args:
            item_name: A configuration item
            user_id: A user ID for the configuration

        Returns:
            If the matched item exists, it is returned. Otherwise, None is
            returned.
        """
        obj = cls.get_object(item_name, user_id)
        if obj is None:
            return None
        return obj.value

    @classmethod
    @transaction.commit_on_success
    def get_items(cls, item_name_list, user_id):
        items = {}
        for item_name in item_name_list:
            value = cls.get(item_name, user_id)
            items[item_name] = value
        return items

    def _store_without_transaction(self):
        obj = self.get_object(self.item_name, self.user_id)
        if obj is not None:
            self.id = obj.id # to update on save()
        self.save()

    @transaction.commit_on_success
    def store(self):
        """Insert if the record with item_name and user_id doesn't exist.
           Otherwise update with value of the this object.
        """
        self._store_without_transaction()

    @classmethod
    @transaction.commit_on_success
    def store_items(cls, items, user_id):
        for name in items:
            value = items[name]
            user_conf = UserConfig(item_name=name, user_id=user_id, value=value)
            user_conf._store_without_transaction()
