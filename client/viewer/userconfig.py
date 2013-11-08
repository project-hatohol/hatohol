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
import urllib2
import httplib
import json
from hatohol import hatoholserver
from hatohol import hatohol_def

def get_user_id_from_hatohol_server(session_id):
    server = hatoholserver.get_address()
    port = hatoholserver.get_port()
    path = '/user/me'
    url = 'http://%s:%d%s' % (server, port, path)
    hdrs = {hatohol_def.FACE_REST_SESSION_ID_HEADER_NAME: session_id}
    req = urllib2.Request(url, headers=hdrs)
    response = urllib2.urlopen(req)
    body = response.read()    
    user_info = json.loads(body)
    # TODO: check the error code
    user_id = user_info['users'][0]['userId']
    return user_id

def index(request, item_name):
    if hatoholserver.SESSION_NAME_META not in request.META:
        return HttpResponse(status=httplib.BAD_REQUEST)
    session_id = request.META[hatoholserver.SESSION_NAME_META]
    user_id = get_user_id_from_hatohol_server(session_id)
    if user_id is None:
        return HttpResponse(status=httplib.UNAUTHORIZED)
    value = UserConfig.get(item_name, user_id)
    body = json.dumps({item_name:value})
    return HttpResponse(body, mimetype='application/json')
