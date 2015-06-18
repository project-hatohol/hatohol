# Create your views here.

import os
import glob
from django.views.generic import TemplateView


class HatoholView(TemplateView):
    plugin_js_files = map(os.path.basename,
                          glob.glob("static/js.plugins/*.js"))

    def get_context_data(self, **kwargs):
        context = super(HatoholView, self).get_context_data(**kwargs)
        context.update({'plugin_js_files': self.plugin_js_files})
        return context
