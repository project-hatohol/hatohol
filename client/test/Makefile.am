if WITH_JS_TEST

TARGET_FILES = \
	node_modules \
	browser/mocha.js browser/mocha.css browser/expect.js browser/sinon.js

noinst_DATA = $(TARGET_FILES)
endif

EXTRA_DIST = \
	__init__.py \
	browser/index.html \
	browser/test_actions_view.js \
	browser/test_events_view.js \
	browser/test_hatohol_connector.js \
	browser/test_hatohol_message_box.js \
	browser/test_hatohol_monitoring_view.js \
	browser/test_hatohol_navi.js \
	browser/test_hatohol_pager.js \
	browser/test_hatohol_reply_parser.js \
	browser/test_hatohol_session_manager.js \
	browser/test_hatohol_user_edit_dialog.js \
	browser/test_hatohol_user_profile.js \
	browser/test_hatohol_user_roles_editor.js \
	browser/test_hatohol_userconfig.js \
	browser/test_incident_settings_view.js \
	browser/test_latest_view.js \
	browser/test_overview_items.js \
	browser/test_history_view.js \
	browser/test_server_view.js \
	browser/test_users_view.js \
	browser/test_utils.js \
	browser/test_log_search_systems_view.js \
	browser/test_graphs_view.js \
	browser/test_triggers_view.js \
	browser/test_hap2_ceilometer.js \
	browser/test_hap2_fluentd.js \
	browser/test_hap2_zabbix.js \
	browser/test_severity_ranks_view.js \
	browser/test_custom_incident_labels_view.js \
	browser/test_hatohol_events_view_config.js \
	feature/dashboard_test.js \
	feature/feature_test_utils.js \
	feature/login_logout_test.js \
	feature/register_action_test.js \
	feature/register_incident_setting_test.js \
	feature/register_log_search_system_test.js \
	feature/register_redmine_test.js \
	feature/register_server_test.js \
	feature/register_user_roles_test.js \
	feature/register_users_test.js \
	feature/run-test.sh \
	python/TestHatoholserver.py \
	python/TestUserConfig.py \
	python/TestUserConfigView.py \
	python/TestLogSearchSystemsView.py \
	python/__init__.py \
	python/run-test.sh \
	python/testsettings.py \
	python/utils.py \
	python/hatohol_server_emulator.py

node_modules:
	ln -sf `npm root`

browser/mocha.js:
	ln -sf ../node_modules/mocha/mocha.js $@

browser/mocha.css:
	ln -sf ../node_modules/mocha/mocha.css $@

browser/expect.js:
	ln -sf ../node_modules/expect.js/index.js $@

browser/sinon.js:
	ln -sf ../node_modules/sinon/pkg/sinon.js $@

clean-local:
	-rm -fr $(TARGET_FILES)
