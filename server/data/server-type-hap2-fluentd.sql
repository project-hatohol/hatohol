INSERT INTO server_types (type, name, parameters, plugin_path, plugin_sql_version, plugin_enabled, uuid)
  VALUES (7, 'Fluentd (HAPI2)', '[{"id": "nickname", "label": "Nickname"}, {"id": "retryInterval", "label": "Retry interval (sec)", "default": "10"}, {"inputStyle": "checkBox", "id": "passiveMode", "label": "Passive mode"}, {"default": "amqp://user:password@localhost/vhost", "id": "brokerUrl", "label": "Broker URL"}, {"hint": "(empty: Default)", "allowEmpty": true, "id": "staticQueueAddress", "label": "Static queue address"}, {"allowEmpty": true, "id": "tlsCertificatePath", "label": "TLS client certificate path"}, {"allowEmpty": true, "id": "tlsKeyPath", "label": "TLS client key path"}, {"allowEmpty": true, "id": "tlsCACertificatePath", "label": "TLS CA certificate path"}, {"inputStyle": "checkBox", "allowEmpty": true, "id": "tlsEnableVerify", "label": "TLS: Enable verify"}]', 'start-stop-hap2-fluentd.sh', 1, 1, 'd91ba1cb-a64a-4072-b2b8-2f91bcae1818')
  ON DUPLICATE KEY UPDATE name='Fluentd (HAPI2)', parameters='[{"id": "nickname", "label": "Nickname"}, {"id": "retryInterval", "label": "Retry interval (sec)", "default": "10"}, {"inputStyle": "checkBox", "id": "passiveMode", "label": "Passive mode"}, {"default": "amqp://user:password@localhost[:port]/vhost", "id": "brokerUrl", "label": "Broker URL"}, {"hint": "(empty: Default)", "allowEmpty": true, "id": "staticQueueAddress", "label": "Static queue address"}, {"allowEmpty": true, "id": "tlsCertificatePath", "label": "TLS client certificate path"}, {"allowEmpty": true, "id": "tlsKeyPath", "label": "TLS client key path"}, {"allowEmpty": true, "id": "tlsCACertificatePath", "label": "TLS CA certificate path"}, {"inputStyle": "checkBox", "allowEmpty": true, "id": "tlsEnableVerify", "label": "TLS: Enable verify"}]', plugin_path='start-stop-hap2-fluentd.sh', plugin_sql_version='1', plugin_enabled='1';
