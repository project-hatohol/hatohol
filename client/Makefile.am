SUBDIRS = . test
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
	hatohol/hatohol_def.py \
	static/css/hatohol.css \
	static/css/zabbix.css \
	static/css.external/themes/ui-lightness/jquery-ui.css \
	static/css.external/themes/ui-lightness/images/ui-bg_gloss-wave_35_f6a828_500x100.png \
	static/css.external/themes/ui-lightness/images/ui-icons_222222_256x240.png \
	static/css.external/themes/ui-lightness/images/ui-bg_glass_100_fdf5ce_1x400.png \
	static/css.external/themes/ui-lightness/images/ui-bg_glass_100_f6f6f6_1x400.png \
	static/css.external/themes/ui-lightness/images/ui-bg_highlight-soft_100_eeeeee_1x100.png \
	static/css.external/themes/ui-lightness/images/ui-bg_diagonals-thick_20_666666_40x40.png \
	static/css.external/themes/ui-lightness/images/ui-bg_glass_65_ffffff_1x400.png \
	static/css.external/themes/ui-lightness/images/ui-icons_ef8c08_256x240.png \
	static/js.external/jquery.js \
	static/js.external/jquery.min.js \
	static/js.external/stupidtable.min.js \
	static/js.external/stupidtable.js \
	static/js.external/jquery-ui.min.js \
	static/js/library.js \
	static/js/hatohol_def.js \
	static/js/hatohol_add_action_dialog.js \
	static/js/hatohol_user_edit_dialog.js \
	static/js/hatohol_dialog.js \
	static/js/hatohol_reply_parser.js \
	static/js/hatohol_server_selector.js \
	static/js/hatohol_host_selector.js \
	static/js/hatohol_selector_dialog.js \
	static/js/hatohol_trigger_selector.js \
	static/js/hatohol_actor_mail_dialog.js \
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
	viewer/actions_ajax.html \
	viewer/users_ajax.html \
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

hatohol/hatohol_def.py: $(HATOHOL_DEF_GENERATOR)
	$(HATOHOL_DEF_GENERATOR) py > $@

update-po:
	./manage.py makemessages -a
	./manage.py makemessages -d djangojs -a

.po.mo:
	./manage.py compilemessages
