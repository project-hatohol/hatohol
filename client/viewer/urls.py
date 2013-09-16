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

from django.conf.urls import patterns, url
from django.views.generic import TemplateView

urlpatterns = patterns('',
    url(r'^$', TemplateView.as_view(template_name='viewer/index.html')),
    url(r'^ajax_dashboard$', TemplateView.as_view(template_name='viewer/dashboard_ajax.html')),
    url(r'^ajax_overview_triggers$', TemplateView.as_view(template_name='viewer/overview_triggers_ajax.html')),
    url(r'^ajax_overview_items$', TemplateView.as_view(template_name='viewer/overview_items_ajax.html')),
    url(r'^ajax_latest$', TemplateView.as_view(template_name='viewer/latest_ajax.html')),
    url(r'^ajax_triggers$', TemplateView.as_view(template_name='viewer/triggers_ajax.html')),
    url(r'^ajax_events$', TemplateView.as_view(template_name='viewer/events_ajax.html')),
    url(r'^ajax_servers$', TemplateView.as_view(template_name='viewer/servers_ajax.html')),
    url(r'^ajax_actions$', TemplateView.as_view(template_name='viewer/actions_ajax.html')),
)
