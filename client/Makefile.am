hatohol_clientdir=$(libexecdir)/$(PACKAGE)/client
nobase_dist_hatohol_client_SCRIPTS = \
	./manage.py

nobase_dist_hatohol_client_DATA = \
	conf/locale/ja/LC_MESSAGES/django.po \
	conf/locale/ja/LC_MESSAGES/djangojs.po \
	hatohol/forwardview.py \
	hatohol/wsgi.py \
	hatohol/urls.py \
	hatohol/__init__.py \
	hatohol/settings.py \
	hatohol/django_realize.py \
	static/css/zabbix.css \
	static/js/library.js \
	static/js/jquery.js \
	static/js/jquery.min.js \
	static/js/stupidtable.min.js \
	static/js/stupidtable.js \
	static/js/hatohol_def.js \
	viewer/urls.py \
	viewer/__init__.py \
	viewer/models.py \
	viewer/triggers_ajax.html \
	viewer/servers_ajax.html \
	viewer/latest_ajax.html \
	viewer/base.html \
	viewer/overview_triggers_ajax.html \
	viewer/views.py \
	viewer/index.html \
	viewer/events_ajax.html \
	viewer/base_ajax.html \
	viewer/overview_items_ajax.html \
	viewer/dashboard_ajax.html \
	viewer/tests.py

MO_FILES = \
	conf/locale/ja/LC_MESSAGES/django.mo \
	conf/locale/ja/LC_MESSAGES/djangojs.mo

CLEANFILES = \
	$(MO_FILES)

nobase_nodist_hatohol_client_DATA = \
	$(MO_FILES)

HATOHOL_DEF_GENERATOR = ../server/tools/hatohol-def-src-file-generator
static/js/hatohol_def.js: $(HATOHOL_DEF_GENERATOR)
	$(HATOHOL_DEF_GENERATOR) js > $@

update-po:
	./manage.py makemessages -a
	./manage.py makemessages -d djangojs -a

.po.mo:
	./manage.py compilemessages
