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

import urllib
import urllib2
from django.http import HttpResponse
import hatoholserver
import hatohol_def

def utf8_dict(src_dict):
    dest_dict = {}
    for key, value in src_dict.iteritems():
        if isinstance(value, unicode):
            value = value.encode('utf8')
        elif isinstance(value, str):
            value.decode('utf8')
        dest_dict[key] = value
    return dest_dict

def jsonforward(request, path):
    server  = hatoholserver.get_address()
    port    = hatoholserver.get_port()
    url     = 'http://%s:%d/%s' % (server, port, path)
    hdrs = {}
    if hatoholserver.SESSION_NAME_META in request.META:
        hdrs = {hatohol_def.FACE_REST_SESSION_ID_HEADER_NAME:
                request.META[hatoholserver.SESSION_NAME_META]}
    if request.method == 'POST':
        encoded_query = urllib.urlencode(utf8_dict(request.POST))
        req = urllib2.Request(url, encoded_query,
                            headers=hdrs)
    elif request.method == 'PUT':
        req = urllib2.Request(url, request.read(),
                            headers=hdrs)
        req.get_method = lambda: 'PUT'
    elif request.method == 'DELETE':
        req = urllib2.Request(url, headers=hdrs)
        req.get_method = lambda: 'DELETE'
    else:
        encoded_query = urllib.urlencode(utf8_dict(request.GET))
        url += '?' + encoded_query
        req = urllib2.Request(url, headers=hdrs)
    content = urllib2.urlopen(req)

    return HttpResponse(content, content_type='application/json')
