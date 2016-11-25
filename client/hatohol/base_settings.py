# Copyright (C) 2013,2015-2016 Project Hatohol
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


# Django settings for hatohol project.
import os
import logging
import ConfigParser
import autotools_vars as av
from branding_settings import *

PROJECT_HOME = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

ADMINS = (
    # ('Your Name', 'your_email@example.com'),
)

MANAGERS = ADMINS

DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.mysql',
        'NAME': 'hatohol_client',
        'USER': 'hatohol',
        'PASSWORD': 'hatohol',
        'HOST': '',                      # Set to empty string for localhost.
        'PORT': '',                      # Set to empty string for default.
    }
}

# Hosts/domain names that are valid for this site; required if DEBUG is False
# See https://docs.djangoproject.com/en//ref/settings/#allowed-hosts
ALLOWED_HOSTS = "*"

# Local time zone for this installation. Choices can be found here:
# http://en.wikipedia.org/wiki/List_of_tz_zones_by_name
# although not all choices may be available on all operating systems.
# In a Windows environment this must be set to your system time zone.
TIME_ZONE = 'America/Chicago'

# Language code for this installation. All choices can be found here:
# http://www.i18nguy.com/unicode/language-identifiers.html
LANGUAGE_CODE = 'en-us'

SITE_ID = 1

# If you set this to False, Django will make some optimizations so as not
# to load the internationalization machinery.
USE_I18N = True

# If you set this to False, Django will not format dates, numbers and
# calendars according to the current locale.
USE_L10N = True

# If you set this to False, Django will not use timezone-aware datetimes.
USE_TZ = True

# Absolute filesystem path to the directory that will hold user-uploaded files.
# Example: "/home/media/media.lawrence.com/media/"
MEDIA_ROOT = ''

# URL that handles the media served from MEDIA_ROOT. Make sure to use a
# trailing slash.
# Examples: "http://media.lawrence.com/media/", "http://example.com/media/"
MEDIA_URL = ''

# Absolute path to the directory static files should be collected to.
# Don't put anything in this directory yourself; store your static files
# in apps' "static/" subdirectories and in STATICFILES_DIRS.
# Example: "/home/media/media.lawrence.com/static/"
STATIC_ROOT = ''

# URL prefix for static files.
# Example: "http://media.lawrence.com/static/"
STATIC_URL = '/static/'

# Additional locations of static files
STATICFILES_DIRS = (
    # Put strings here, like "/home/html/static" or "C:/www/django/static".
    # Always use forward slashes, even on Windows.
    # Don't forget to use absolute paths, not relative paths.
    os.path.join(PROJECT_HOME, 'static'),
)

# List of finder classes that know how to find static files in
# various locations.
STATICFILES_FINDERS = (
    'django.contrib.staticfiles.finders.FileSystemFinder',
    'django.contrib.staticfiles.finders.AppDirectoriesFinder',
    # 'django.contrib.staticfiles.finders.DefaultStorageFinder',
)

# Make this unique, and don't share it with anybody.
SECRET_KEY = '9xkobcf%(rj(u54^zf7+-c^@+59c9dqg&amp;he2ue65v88z&amp;dyy_k'

# List of callables that know how to import templates from various sources.
TEMPLATE_LOADERS = (
    'django.template.loaders.filesystem.Loader',
    'django.template.loaders.app_directories.Loader',
    # 'django.template.loaders.eggs.Loader',
)

MIDDLEWARE_CLASSES = (
    'django.middleware.common.CommonMiddleware',
    'django.middleware.csrf.CsrfViewMiddleware',
    'django.middleware.locale.LocaleMiddleware',
    # Uncomment the next line for simple clickjacking protection:
    # 'django.middleware.clickjacking.XFrameOptionsMiddleware',
)

ROOT_URLCONF = 'hatohol.urls'

# Python dotted path to the WSGI application used by Django's runserver.
WSGI_APPLICATION = 'hatohol.wsgi.application'

TEMPLATE_DIRS = (
    # Put strings here, like "/home/html/django_templates" or
    # "C:/www/django/templates".
    # Always use forward slashes, even on Windows.
    # Don't forget to use absolute paths, not relative paths.
    PROJECT_HOME,
)

INSTALLED_APPS = (
    # 'django.contrib.auth',
    # 'django.contrib.contenttypes',
    # 'django.contrib.sessions',
    # 'django.contrib.sites',
    # 'django.contrib.messages',
    'django.contrib.staticfiles',
    # Uncomment the next line to enable the admin:
    # 'django.contrib.admin',
    # Uncomment the next line to enable admin documentation:
    # 'django.contrib.admindocs',
    'hatohol',
)

# A sample logging configuration. The only tangible logging
# performed by this configuration is to send an email to
# the site admins on every HTTP 500 error when DEBUG=False.
# See http://docs.djangoproject.com/en/dev/topics/logging for
# more details on how to customize your logging configuration.
LOGGING = {
    'version': 1,
    'disable_existing_loggers': False,
    'filters': {
        'require_debug_false': {
            '()': 'django.utils.log.RequireDebugFalse'
        }
    },
    'handlers': {
        'mail_admins': {
            'level': 'ERROR',
            'filters': ['require_debug_false'],
            'class': 'django.utils.log.AdminEmailHandler'
        },
        'console': {
            'level': 'WARNING',
            'class': 'logging.StreamHandler',
        },
        'syslog': {
            'level': 'INFO',
            'class': 'logging.handlers.SysLogHandler'
        }
    },
    'loggers': {
        'django.request': {
            'handlers': ['mail_admins'],
            'level': 'ERROR',
            'propagate': True,
        },
        'hatohol': {
            'handlers': ['syslog', 'console'],
            'level': 'INFO',
            'propagate': True,
        },
        'viewer': {
            'handlers': ['syslog', 'console'],
            'level': 'INFO',
            'propagate': True,
        }
    }
}

LOCALE_PATHS = (
    os.path.join(PROJECT_HOME, "conf", "locale"),
)

# The list of enabled pages. Specified pages are shown in the top navbar.
# When the list is empty, all pages are shown.
ENABLED_PAGES = (
    #
    # monitoring views
    #
    "ajax_dashboard",
    "ajax_overview_triggers",
    #"ajax_overview_items",
    "ajax_latest",
    "ajax_triggers",
    "ajax_events",

    #
    # settings views
    #
    "ajax_servers",
    "ajax_actions",
    "ajax_graphs",
    "ajax_incident_settings",
    "ajax_log_search_systems",
    "ajax_users",
    "ajax_severity_ranks",
    "ajax_custom_incident_labels",

    #
    # Help menu
    #
    "http://www.hatohol.org/docs",
    "#version",
)

def get_config_lines_from_file():
    conf_lines = []

    config = ConfigParser.ConfigParser()
    selected = config.read(['webui.conf',
                            av.SYSCONFDIR + '/hatohol/webui.conf'])
    if len(selected) == 0:
        return conf_lines

    config_defs = (
        ('generic', {
            'allowed_hosts': lambda v: 'ALLOWED_HOSTS = %s' % v.split(),
            'time_zone':     lambda v: 'TIME_ZONE = %s' % v,
        }),
        ('database', {
            'name': lambda v: 'DATABASES["default"]["NAME"] = "%s"' % v,
            'user': lambda v: 'DATABASES["default"]["USER"] = "%s"' % v,
            'password': lambda v: 'DATABASES["default"]["PASSWORD"] = "%s"' % v,
            'host': lambda v: 'DATABASES["default"]["HOST"] = "%s"' % v,
            'port': lambda v: 'DATABASES["default"]["PORT"] = "%s"' % v,
        }),
        ('server', {
            'host': lambda v: 'HATOHOL_SERVER_ADDR = "%s"' % v,
            'port': lambda v: 'HATOHOL_SERVER_PORT = %s' % v,
        }),
    )

    for section, handlers in config_defs:
        for (key, val) in config.items(section):
            handler = handlers.get(key)
            if handler:
                conf_lines.append(handler(val))
            else:
                print 'Unknown keyword: [%s] %s (%s)' % (section, key, val)
    return conf_lines

for line in get_config_lines_from_file():
    exec(line)
