import os
import sys

path = '/path/to/hatohol'
if path not in sys.path:
    sys.path.append(path)

os.environ['DJANGO_SETTINGS_MODULE'] = 'hatohol.settings'

import django.core.handlers.wsgi
application = django.core.handlers.wsgi.WSGIHandler()
