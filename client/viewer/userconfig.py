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

from django.http import HttpResponse
from hatohol.models import UserConfig
import json

def get_user_id_from_hatohol_server(session_id):
    return 0; # TODO: get ID from hatohol server

def index(request, item_name):
    if "HTTP_X_HATOHOL_SESSION" not in request.META:
        return HttpResponse('Bad Request', status=400)
    session_id = request.META["HTTP_X_HATOHOL_SESSION"]
    user_id = get_user_id_from_hatohol_server(session_id):
    if user_id is None:
        return HttpResponse('Unauthorized', status=401)
    value = UserConfig.get(item_name, user_id)
    body = json.dumps({item_name:value})
    return HttpResponse(body, mimetype='application/json')
