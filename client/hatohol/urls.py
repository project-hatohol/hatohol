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

def guessContentTypeFromFileName(file_name):
    if re.search('\.js$', file_name):
        return 'text/javascript'
    elif re.search('\.css$', file_name):
        return 'text/css'
    return 'text/html'

def makeTastingUrl(file_name):
    content_type = guessContentTypeFromFileName(file_name)
    return url(r'^tasting/' + file_name + '$',
               TemplateView.as_view(template_name='tasting/' + file_name,
                                    content_type=content_type))

def makeTestUrl(file_name):
    content_type = guessContentTypeFromFileName(file_name)
    return url(r'^test/' + file_name + '$',
               TemplateView.as_view(template_name='test/browser/' + file_name,
                                    content_type=content_type))

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
    url(r'^userconfig/(?P<item_name>.+)/', 'viewer.userconfig.index'),
    url(r'^jsi18n/$', 'django.views.i18n.javascript_catalog'),
)

urlpatterns += i18n_patterns('',
    url(r'^viewer/', include('viewer.urls')),
    url(r'^jsi18n/$', 'django.views.i18n.javascript_catalog'),
)

if 'HATOHOL_DEBUG' in os.environ and os.environ['HATOHOL_DEBUG'] == '1':
    import test.python.utils

    urlpatterns += patterns('',
        makeTastingUrl('index.html'),
        makeTastingUrl('hatohol_login_dialog.html'),
        makeTastingUrl('hatohol_message_box.html'),
        makeTastingUrl('hatohol_session_manager.html'),
        makeTastingUrl('hatohol_connector.html'),
        makeTastingUrl('js_loader.js'),
        url(r'^test/hello', test.python.utils.hello),
        makeTestUrl('index.html'),
        makeTestUrl('test_hatohol_session_manager.js'),
        makeTestUrl('test_hatohol_connector.js'),
        makeTestUrl('test_hatohol_message_box.js'),
        makeTestUrl('mocha.js'),
        makeTestUrl('mocha.css'),
        makeTestUrl('expect.js'),
    )

