# Create your views here.

import os
import glob
from django.conf import settings
from django.views.generic import TemplateView


class HatoholView(TemplateView):
    plugin_js_dir = os.path.join(settings.PROJECT_HOME, "static", "js.plugins")
    plugin_js_files = map(os.path.basename,
                          glob.glob(os.path.join(plugin_js_dir, "*.js")))

    def get_context_data(self, **kwargs):
        context = super(HatoholView, self).get_context_data(**kwargs)
        context.update({
            'brand_name' : settings.BRAND_NAME,
            'vendor_name' : settings.VENDOR_NAME,
            'enabled_pages': settings.ENABLED_PAGES,
            'plugin_js_files': self.plugin_js_files,
            'project_top_path': self.request.META.get("SCRIPT_NAME", ""),
        })
        return context
