DELETE FROM server_types;
INSERT server_types VALUES (0, 'Zabbix', '[{"name":"Nickname"},{"name":"Host name"},{"name":"IP address"}]');
INSERT server_types VALUES (1, 'Nagios', '[{"name":"Nickname"}]');
INSERT server_types VALUES (2, 'Zabbix (HAPI)', '[{"name":"Nickname"},{"name":"Host name"}]');
