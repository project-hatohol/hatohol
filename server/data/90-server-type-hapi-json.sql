INSERT INTO server_types (type, name, parameters, plugin_path, plugin_sql_version, plugin_enabled, uuid) VALUES (4, 'JSON (HAPI)', '[{"id": "nickname", "label": "Nickname"}, {"hint": "(empty: Default)", "allowEmpty": true, "id": "brokerUrl", "label": "Broker URL"}, {"hint": "(empty: Default)", "allowEmpty": true, "id": "staticQueueAddress", "label": "Static queue address"}, {"allowEmpty": true, "id": "tlsCertificatePath", "label": "TLS client certificate path"}, {"allowEmpty": true, "id": "tlsKeyPath", "label": "TLS client key path"}, {"allowEmpty": true, "id": "tlsCACertificatePath", "label": "TLS CA certificate path"}, {"inputStyle": "checkBox", "allowEmpty": true, "id": "tlsEnableVerify", "label": "TLS: Enable verify"}]', '', 1, 1, '') ON DUPLICATE KEY UPDATE name='JSON (HAPI)', parameters='[{"id": "nickname", "label": "Nickname"}, {"hint": "(empty: Default)", "allowEmpty": true, "id": "brokerUrl", "label": "Broker URL"}, {"hint": "(empty: Default)", "allowEmpty": true, "id": "staticQueueAddress", "label": "Static queue address"}, {"allowEmpty": true, "id": "tlsCertificatePath", "label": "TLS client certificate path"}, {"allowEmpty": true, "id": "tlsKeyPath", "label": "TLS client key path"}, {"allowEmpty": true, "id": "tlsCACertificatePath", "label": "TLS CA certificate path"}, {"inputStyle": "checkBox", "allowEmpty": true, "id": "tlsEnableVerify", "label": "TLS: Enable verify"}]', plugin_path='', plugin_sql_version='1', plugin_enabled='1', uuid='';