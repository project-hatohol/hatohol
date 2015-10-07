import os

os.environ['DJANGO_SETTINGS_MODULE'] = 'hatohol.settings_product'

import django.core.handlers.wsgi
application = django.core.handlers.wsgi.WSGIHandler()
