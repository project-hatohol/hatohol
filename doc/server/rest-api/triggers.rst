=========================
GET Trigger
=========================

Return a list of `Trigger object`_ aggregated by the Hatohol server from each
monitoring servers.

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
   * - /trigger

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
   * - limit
     - Specifies a limit for the number of triggers to retrieve.
     - Optional
   * - offset
     - Specifies the number of triggers to skip before returning triggers.
     - Optional
   * - serverId
     - Specifies a monitoring server's ID to retrieve triggers belong to it.
     - Optional
   * - hostgroupId
     - Specifies a host group ID to retrieve triggers belong to it.
     - Optional
   * - hostId
     - Specifies a host ID to retrieve triggers belong to it.
     - Optional
   * - minimumSeverity
     - Specifies the minimum :ref:`trigger-severity` to retrieve triggers.
     - Optional
   * - status
     - Any of :ref:`trigger-status`.
     - Optional

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
   * - triggers
     - Array
     - The array of `Trigger object`_.
     - True
   * - numberOfTriggers
     - Number
     - The number of triggers in the `triggers` array.
     - True
   * - totalNumberOfTriggers
     - Number
     - The total number of triggers in the Hatohol's DB.
     - True
   * - servers
     - Object
     - List of :ref:`server-object`. Keys for each :ref:`server-object` are server IDs which corresponds to serverId values in `Trigger object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

.. _trigger-object:

Trigger object
~~~~~~~~~~~~~~~~~~~

`Trigger object`_ represents a source of event. This concept is inspired by
Zabbix. In Zabbix it contains a set of conditions to generate events although
Hatohol doesn't use it. Since it also contains the state of last event, it
indicates the current sate of it.

.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - id
     - String
     - N/A
   * - status
     - Number
     - A `Trigger status`_.
   * - severity
     - Number
     - A `Trigger severity`_.
   * - lastChangeTime
     - Number
     - A last change time of the trigger.
   * - serverId
     - Number
     - A server ID.
   * - hostId
     - String
     - A host ID.
   * - brief
     - String
     - A brief of the trigger.
   * - extendedInfo
     - String
     - Additional data depend on the monitoring system.

.. _trigger-status:

Trigger status
~~~~~~~~~~~~~~~~~~~
.. list-table::

   * - 0
     - TRIGGER_STATUS_OK
   * - 1
     - TRIGGER_STATUS_PROBLEM
   * - 2
     - TRIGGER_STATUS_UNKNOWN

.. _trigger-severity:

Trigger severity
~~~~~~~~~~~~~~~~~~~
.. list-table::

   * - 0
     - TRIGGER_SEVERITY_UNKNOWN
   * - 1
     - TRIGGER_SEVERITY_INFO
   * - 2
     - TRIGGER_SEVERITY_WARNING
   * - 3
     - TRIGGER_SEVERITY_ERROR
   * - 4
     - TRIGGER_SEVERITY_CRITICAL
   * - 5
     - TRIGGER_SEVERITY_EMERGENCY

Example
-------------
.. code-block:: json

  {
    "apiVersion":4,
    "errorCode":0,
    "triggers":[
      {
        "id":"13517",
        "status":0,
        "severity":1,
        "lastChangeTime":0,
        "serverId":4,
        "hostId":"10085",
        "brief":"Host name of zabbix_agentd was changed on {HOST.NAME}",
        "extendedInfo":"{\"expandedDescription\":\"Host name of zabbix_agentd was changed on debian\"}"
      },
      {
        "id":"13518",
        "status":0,
        "severity":3,
        "lastChangeTime":1450557238,
        "serverId":4,
        "hostId":"10085",
        "brief":"Zabbix agent on {HOST.NAME} is unreachable for 5 minutes",
        "extendedInfo":"{\"expandedDescription\":\"Zabbix agent on debian is unreachable for 5 minutes\"}"
      },
      {
        "id":"13519",
        "status":0,
        "severity":1,
        "lastChangeTime":1399120139,
        "serverId":4,
        "hostId":"10085",
        "brief":"Version of zabbix_agent(d) was changed on {HOST.NAME}",
        "extendedInfo":"{\"expandedDescription\":\"Version of zabbix_agent(d) was changed on debian\"}"
      },
      {
        "id":"13520",
        "status":0,
        "severity":1,
        "lastChangeTime":1399120140,
        "serverId":4,
        "hostId":"10085",
        "brief":"Configured max number of opened files is too low on {HOST.NAME}",
        "extendedInfo":"{\"expandedDescription\":\"Configured max number of opened files is too low on debian\"}"
      },
      {
        "id":"13521",
        "status":0,
        "severity":1,
        "lastChangeTime":1399120141,
        "serverId":4,
        "hostId":"10085",
        "brief":"Configured max number of processes is too low on {HOST.NAME}",
        "extendedInfo":"{\"expandedDescription\":\"Configured max number of processes is too low on debian\"}"
      }
    ],
    "numberOfTriggers":5,
    "totalNumberOfTriggers":140,
    "servers":{
      "4":{
        "name":"Zabbix",
        "nickname":"zabbix",
        "type":0,
        "ipAddress":"192.168.1.10",
        "baseURL":"",
        "hosts":{
          "10085":{
            "name":"debian"
          },
          "__SELF_MONITOR":{
            "name":"Zabbix_SELF"
          }
        },
        "groups":{
          "4":{
            "name":"Zabbix servers"
          },
          "6":{
            "name":"HTTP servers"
          }
        }
      },
      "5":{
        "name":"HAPI2 Zabbix",
        "nickname":"HAPI2 Zabbix",
        "type":7,
        "ipAddress":"",
        "baseURL":"http://192.168.1.11/zabbix/api_jsonrpc.php",
        "uuid":"8e632c14-d1f7-11e4-8350-d43d7e3146fb",
        "hosts":{
          "10084":{
            "name":"Zabbix server"
          },
          "__SELF_MONITOR":{
            "name":"(self-monitor)"
          }
        },
        "groups":{
          "2":{
            "name":"Linux servers"
          },
          "4":{
            "name":"Zabbix servers"
          }
        }
      }
    }
  }
