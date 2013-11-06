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
import smartfield

class UserConfig(models.Model):
    item_name = models.CharField(max_length=255, db_index=True)
    user_id = models.IntegerField(db_index=True)
    value = smartfield.SmartField()

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
        objs = UserConfig.objects.filter(item_name=item_name).filter(user_id=user_id)
        if not objs:
            return None
        assert len(objs) == 1
        return objs[0]

