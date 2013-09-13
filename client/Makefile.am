libexecdir=$(exec_prefix)/libexec/$(PACKAGE)/client
nobase_dist_libexec_SCRIPTS = \
	./manage.py \
	conf/locale/ja/LC_MESSAGES/django.po \
	conf/locale/ja/LC_MESSAGES/djangojs.po \
	mysite/forwardview.py \
	mysite/wsgi.py \
	mysite/urls.py \
	mysite/__init__.py \
	mysite/settings.py \
	mysite/django_realize.py \
	static/css/zabbix.css \
	static/js/library.js \
	static/js/jquery.js \
	static/js/jquery.min.js \
	static/js/stupidtable.min.js \
	static/js/stupidtable.js \
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

nobase_nodist_libexec_SCRIPTS = \
	$(MO_FILES)

update-po:
	./manage.py makemessages -a
	./manage.py makemessages -d djangojs -a

.po.mo:
	./manage.py compilemessages
