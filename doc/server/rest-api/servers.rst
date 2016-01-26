=========================
GET Server
=========================

Resturn a list of `Server object`_.

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
   * - /server


Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Brief
     - Condition
   * - fmt
     - Specifies the format of the returned data. "json" or "jsonp" are valid.
       If this parameter is omitted, the default value is "json".
     - | Optional for JSON ("json").
       | Mandatory for JSONP ("jsonp").
   * - callback
     - The name of the returned JSONP object.
     - | N/A for JSON.
       | Mandatory for JSONP.

Response
========

Response structure
------------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
     - Condition
   * - apiVersion
     - Number
     - An API version of this URL.
       This document is written for version **4**.
     - Always
   * - errorCode
     - Number
     - 0 on success, non-0 error code otherwise.
     - Always
   * - errorMessage
     - String
     - An error message.
     - False
   * - numberOfServers
     - Number
     - The number of monitoring servers.
     - True
   * - servers
     - Array
     - The array of `Server object`_.
       server.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Server object
~~~~~~~~~~~~~~~~~~~

`Server object`_ contains properties (such as settings for communicating with
the server) of a monitoring server.

.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
     - Condition
   * - id
     - Number
     - An unique ID of the monitoring server.
     - Mandatory
   * - type
     - Number
     - Any of :ref:`server-type`.
     - Mandatory
   * - uuid
     - String
     - An UUID of the HAP2 plugin for the monitoring system.
     - When the `type` is `7 (HAP2 plugin)`.
   * - nickname
     - String
     - Arbitrary nickname of the monitoring server.
     - Mandatory
   * - hostName
     - String
     - A host name of the monitoring server.
     - Optional
   * - ipAddress
     - String
     - An IP Address of the monitoring server.
     - Optional
   * - port
     - Number
     - A port number of the monitoring server.
     - Optional
   * - pollingInterval
     - Number
     - Polling interval in seconds.
     - Optional
   * - retryInterval
     - Number
     - Retry interval on failing communication with the server (in seconds).
     - Optional
   * - baseURL
     - String
     - An URL of the monitoring server.
     - Optional
   * - extendedInfo
     - String
     - Additional data depend on the monitoring system.
     - Optional
   * - userName
     - String
     - A user name.
     - Optional
   * - password
     - String
     - A password.
     - Optional
   * - dbName
     - String
     - A DB name.
     - Optional
   * - passiveMode
     - Boolean
     - N/A
     - When the `type` is `7 (HAP2 plugin)`.
   * - brokerUrl
     - String
     - A URL of AMQP broker.
     - When the `type` is `7 (HAP2 plugin)` or 4 `(HAPI JSON)`.
   * - staticQueueAddress
     - String
     - A queue name of AMQP. When the value is empty it's determined by Hatohol
       server.
     - When the `type` is `7 (HAP2 plugin)`.
   * - tlsCertificatePath
     - String
     - TLS client certificate path.
     - When the `type` is `7 (HAP2 plugin)` or 4 `(HAPI JSON)`.
   * - tlsKeyPath
     - String
     - TLS client key path.
     - When the `type` is `7 (HAP2 plugin)` or 4 `(HAPI JSON)`.
   * - tlsCACertificatePath
     - String
     - TLS CA certificate path.
     - When the `type` is `7 (HAP2 plugin)` or 4 `(HAPI JSON)`.
   * - tlsEnableVerify
     - Boolean
     - Specifies to enable TLS verify or not.
     - When the `type` is `7 (HAP2 plugin)` or 4 `(HAPI JSON)`.

.. note:: [Condition] Mandatory: Always provided, Optional: Depends on the monitoring system.

Example
-----------------
.. code-block:: json

  {
    "apiVersion":4,
    "errorCode":0,
    "numberOfServers":2,
    "servers":[
      {
        "id":4,
        "type":0,
        "hostName":"Zabbix",
        "ipAddress":"192.168.1.10",
        "nickname":"zabbix",
        "port":80,
        "pollingInterval":30,
        "retryInterval":10,
        "baseURL":"",
        "extendedInfo":"",
        "userName":"Admin",
        "password":"xxxxxx",
        "dbName":""
      },
      {
        "id":5,
        "type":7,
        "hostName":"HAPI2 Zabbix",
        "ipAddress":"",
        "nickname":"HAPI2 Zabbix",
        "port":0,
        "pollingInterval":30,
        "retryInterval":10,
        "baseURL":"http://192.168.1.11/zabbix/api_jsonrpc.php",
        "extendedInfo":"",
        "userName":"Admin",
        "password":"xxxxxx",
        "dbName":"",
        "passiveMode":false,
        "brokerUrl":"amqp://hatohol:hatohol@localhost/hatohol",
        "staticQueueAddress":"",
        "tlsCertificatePath":"",
        "tlsKeyPath":"",
        "tlsCACertificatePath":"",
        "uuid":"8e632c14-d1f7-11e4-8350-d43d7e3146fb",
        "tlsEnableVerify":false
      }
    ]
  }
