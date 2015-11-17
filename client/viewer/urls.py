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

from hatohol import hatohol_def
from views import HatoholView

urlpatterns = patterns(
    '',
    url(r'^$',
        HatoholView.as_view(template_name='viewer/dashboard_ajax.html')),
    url(r'^ajax_dashboard$',
        HatoholView.as_view(
            template_name='viewer/dashboard_ajax.html')),
    url(r'^ajax_overview_triggers$',
        HatoholView.as_view(
            template_name='viewer/overview_triggers_ajax.html')),
    url(r'^ajax_overview_items$',
        HatoholView.as_view(
            template_name='viewer/overview_items_ajax.html')),
    url(r'^ajax_latest$',
        HatoholView.as_view(template_name='viewer/latest_ajax.html')),
    url(r'^ajax_triggers$',
        HatoholView.as_view(template_name='viewer/triggers_ajax.html')),
    url(r'^ajax_events$',
        HatoholView.as_view(template_name='viewer/events_ajax.html')),
    url(r'^ajax_servers$',
        HatoholView.as_view(template_name='viewer/servers_ajax.html'),
        kwargs={'hatohol': hatohol_def}),
    url(r'^ajax_actions$',
        HatoholView.as_view(template_name='viewer/actions_ajax.html')),
    url(r'^ajax_incident_settings$',
        HatoholView.as_view(
            template_name='viewer/incident_settings_ajax.html')),
    url(r'^ajax_log_search_systems$',
        HatoholView.as_view(
            template_name='viewer/log_search_systems_ajax.html')),
    url(r'^ajax_graphs$',
        HatoholView.as_view(template_name='viewer/graphs_ajax.html')),
    url(r'^ajax_users$',
        HatoholView.as_view(template_name='viewer/users_ajax.html')),
    url(r'^ajax_hosts$',
        HatoholView.as_view(template_name='viewer/hosts_ajax.html')),
    url(r'^ajax_history$',
        HatoholView.as_view(template_name='viewer/history_ajax.html')),
    url(r'^ajax_severity_ranks$',
        HatoholView.as_view(template_name='viewer/severity_ranks_ajax.html')),
    url(r'^ajax_custom_incident_labels$',
        HatoholView.as_view(template_name='viewer/custom_incident_labels_ajax.html')),
)
