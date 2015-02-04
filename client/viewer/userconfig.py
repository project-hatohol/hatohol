# Copyright (C) 2013 Project Hatohol
#
# This file is part of Hatohol.
#
# Hatohol is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License, version 3
# as published by the Free Software Foundation.
#
# Hatohol is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with Hatohol. If not, see
# <http://www.gnu.org/licenses/>.

from django.http import HttpResponse
from hatohol.models import UserConfig
import urllib2
import httplib
import json
from hatohol import hatoholserver
from hatohol import hatohol_def
import logging
import traceback

logger = logging.getLogger(__name__)
logger.info('A logger: %s has been created' % __name__)

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
    user_id = user_info['users'][0]['userId']
    return user_id

def index(request):
    try:
        return index_core(request)
    except:
        logger.error(traceback.format_exc())
        return HttpResponse(status=httplib.INTERNAL_SERVER_ERROR)

def index_core(request):
    # session ID
    if hatoholserver.SESSION_NAME_META not in request.META:
        logger.info('Session ID is missing.')
        return HttpResponse(status=httplib.BAD_REQUEST)
    session_id = request.META[hatoholserver.SESSION_NAME_META]

    user_id = get_user_id_from_hatohol_server(session_id)
    if user_id is None:
        logger.info('Failed to get user ID.')
        return HttpResponse(status=httplib.UNAUTHORIZED)

    # dispatch
    if request.method == 'POST':
        return store(request, user_id)

    # keys
    if not request.GET.has_key('items[]'):
        logger.info('Not found key: items[].')
        return HttpResponse(status=httplib.BAD_REQUEST)
    item_name_list = request.GET.getlist('items[]')

    body = json.dumps(UserConfig.get_items(item_name_list, user_id))
    return HttpResponse(body, content_type = 'application/json')

def store(request, user_id):
    items = json.loads(request.body)
    UserConfig.store_items(items, user_id)
    return HttpResponse()

