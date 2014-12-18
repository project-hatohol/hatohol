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

from django.conf.urls import patterns, url
from django.views.generic import TemplateView

from hatohol import hatohol_def

urlpatterns = patterns('',
    url(r'^$', TemplateView.as_view(template_name='viewer/dashboard_ajax.html')),
    url(r'^ajax_dashboard$', TemplateView.as_view(template_name='viewer/dashboard_ajax.html')),
    url(r'^ajax_overview_triggers$', TemplateView.as_view(template_name='viewer/overview_triggers_ajax.html')),
    url(r'^ajax_overview_items$', TemplateView.as_view(template_name='viewer/overview_items_ajax.html')),
    url(r'^ajax_latest$', TemplateView.as_view(template_name='viewer/latest_ajax.html')),
    url(r'^ajax_triggers$', TemplateView.as_view(template_name='viewer/triggers_ajax.html')),
    url(r'^ajax_events$', TemplateView.as_view(template_name='viewer/events_ajax.html')),
    url(r'^ajax_servers$', TemplateView.as_view(template_name='viewer/servers_ajax.html'), kwargs={'hatohol': hatohol_def}),
    url(r'^ajax_actions$', TemplateView.as_view(template_name='viewer/actions_ajax.html')),
    url(r'^ajax_incident_settings$', TemplateView.as_view(template_name='viewer/incident_settings_ajax.html')),
    url(r'^ajax_users$', TemplateView.as_view(template_name='viewer/users_ajax.html')),
    url(r'^ajax_hosts$', TemplateView.as_view(template_name='viewer/hosts_ajax.html')),
)
