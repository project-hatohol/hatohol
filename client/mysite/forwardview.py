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

def jsonforward(request, path, **kwargs):
    server  = kwargs['server']
    url     = 'http://%s/%s' % (server, path)
    if request.method == "POST":
      content = urllib2.urlopen(url, urllib.urlencode(request.REQUEST))
    else:
      content = urllib2.urlopen(url)

    return HttpResponse(content,
                        content_type='application/json')
