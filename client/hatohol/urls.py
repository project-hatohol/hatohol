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

import os
import re
from django.conf.urls import patterns, include, url
from django.conf.urls.i18n import i18n_patterns
from hatohol.forwardview import jsonforward
from django.views.generic import TemplateView

# Uncomment the next two lines to enable the admin:
# from django.contrib import admin
# admin.autodiscover()

def guessContentTypeFromExt(ext):
    if ext is "js":
        return 'text/javascript'
    elif ext is "css":
        return 'text/css'
    return 'text/html'

def staticFile(request, prefix, path, ext):
    content_type = guessContentTypeFromExt(ext)
    view = TemplateView.as_view(template_name=prefix + path,
                                content_type=content_type)
    return view(request)

def tastingFile(request, path, ext):
    return staticFile(request, "tasting/", path, ext)

def testFile(request, path, ext):
    return staticFile(request, "test/browser/", path, ext)

urlpatterns = patterns('',
    # Examples:
    # url(r'^$', 'hatohol.views.home', name='home'),
    # url(r'^hatohol/', include('hatohol.foo.urls')),

    # Uncomment the admin/doc line below to enable admin documentation:
    # url(r'^admin/doc/', include('django.contrib.admindocs.urls')),

    # Uncomment the next line to enable the admin:
    # url(r'^admin/', include(admin.site.urls)),
    url(r'^viewer/', include('viewer.urls')),
    url(r'^tunnel/(?P<path>.+)', jsonforward),
    url(r'^userconfig$', 'viewer.userconfig.index'),
    url(r'^jsi18n/$', 'django.views.i18n.javascript_catalog'),
)

urlpatterns += i18n_patterns('',
    url(r'^viewer/', include('viewer.urls')),
    url(r'^jsi18n/$', 'django.views.i18n.javascript_catalog'),
)

if 'HATOHOL_DEBUG' in os.environ and os.environ['HATOHOL_DEBUG'] == '1':
    import test.python.utils

    urlpatterns += patterns('',
        url(r'^tasting/(.+\.(js|css|html))$', tastingFile),
        url(r'^test/hello', test.python.utils.hello),
        url(r'^test/delete_user_config', test.python.utils.delete_user_config),
        url(r'^test/(.+\.(js|css|html))$', testFile),
    )

