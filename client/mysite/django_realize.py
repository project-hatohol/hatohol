# django-realize 0.1
# by Liu Qishuai <lqs.buaa@gmail.com>
# This file is released under the GNU Lesser General Public License. 

import django
import time
import datetime

native_types = (int, long, float, str)
list_types = (tuple, list, set, django.db.models.query.QuerySet, django.forms.util.ErrorList)
dict_types = (dict, django.forms.util.ErrorDict)
unicode_types = (unicode, django.utils.functional.Promise)
object_types = (django.db.models.Model, django.db.models.manager.Manager, django.forms.models.BaseForm)

class InvalidType:
    pass

def valid(obj):
    return obj != InvalidType
    
def realize(obj, level = 3):
    res = InvalidType

    if level == 0:
        res = InvalidType
    elif any([isinstance(obj, t) for t in native_types]):
        res = obj
    elif any([isinstance(obj, t) for t in list_types]):
        res = filter(valid, [realize(sub, level - 1) for sub in list(obj)])
    elif any([isinstance(obj, t) for t in dict_types]):
        res = {}
        for k in dict(obj):
            res[k] = realize(obj[k])
    elif isinstance(obj, datetime.datetime):
        res = time.mktime(obj.timetuple())
    elif any([isinstance(obj, t) for t in object_types]):
        res = {}
        for key in dir(obj):
            # some attributes may be unreadable
            try:
                if key[0] != '_':
                    v = realize(getattr(obj, key), level - 1)
                    if v != InvalidType:
                        res[key] = v
            except:
                pass
    elif any([isinstance(obj, t) for t in unicode_types]):
        res = unicode(obj)
    return res
