=========================
GET Overview
=========================

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
   * - /overview

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
     - The total number of monitoring servers.
     - True
   * - serverStatus
     - Object
     - The array of `ServerStatus object`_ which contains various statuses of a
       monitoring srever.
     - True
   * - numberOfGoodServers
     - Number
     - The number of monitoring servers which don't have bad hosts.
     - True
   * - numberOfBadServers
     - Number
     - The number of monitoring servers which have bad hosts.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

ServerStatus object
~~~~~~~~~~~~~~~~~~~
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - serverId
     - Number
     - An ID of the monitoring server.
   * - serverHostName
     - String
     - A host name of the monitoring server.
   * - serverIpAddr
     - String
     - An IP address of the monitoring server.
   * - serverNickname
     - String
     - A nickname of the monitoring server.
   * - numberOfHosts
     - Number
     - The number of hosts.
   * - numberOfItems
     - Number
     - The number of items.
   * - numberOfTriggers
     - Number
     - The number of triggers.
   * - numberOfBadHosts
     - Number
     - The number of bad hosts.
   * - numberOfBadTriggers
     - Number
     - The number of bad triggers.
   * - numberOfUsers
     - Number
     - | The number of users.
       | **Currently not implemented ('0' is returned)**.
   * - numberOfOnlineUsers
     - Number
     - | The number of online users.
       | **Currently not implemented ('0' is returned)**.
   * - numberOfMonitoredItemsPerSecond
     - Number
     - | The number of monitored items per second.
   * - hostGroups
     - Object
     - List of `HostGroup object`_. Keys for each `HostGroup object`_ are host group IDs. 
   * - systemStatus
     - Array
     - The array of `SystemStatus object`_.
   * - hostStatus
     - Array
     - The array of `HostStatus object`_.

HostGroup object
~~~~~~~~~~~~~~~~~~~
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A name of the host group.

SystemStatus object
~~~~~~~~~~~~~~~~~~~
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - hostgroupId
     - Number
     - A host groud ID.
   * - severity
     - Number
     - A :ref:`trigger-severity`.
   * - numberOfTriggers
     - Number
     - The number of triggers.

HostStatus object
~~~~~~~~~~~~~~~~~~~
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - hostgroupId
     - Number
     - A host groud ID.
   * - numberOfGoodHosts
     - Number
     - The number of good hosts whose host group ID is hostGroupId in this object.
   * - numberOfBadHosts
     - Number
     - A number of bad hosts whose host group ID is hostGroupId in this object.


Example
-----------------
.. code-block:: json

  {
    "apiVersion":4,
    "errorCode":0,
    "numberOfServers":1,
    "serverStatus":[
      {
        "serverId":4,
        "serverHostName":"Zabbix",
        "serverIpAddr":"192.168.1.10",
        "serverNickname":"zabbix",
        "numberOfHosts":3,
        "numberOfItems":90,
        "numberOfTriggers":68,
        "numberOfBadHosts":2,
        "numberOfBadTriggers":2,
        "numberOfUsers":0,
        "numberOfOnlineUsers":0,
        "numberOfMonitoredItemsPerSecond":"1.20",
        "hostgroups":{
          "2":{
            "name":"Linux servers"
          },
          "4":{
            "name":"Zabbix servers"
          },
          "6":{
            "name":"HTTP servers"
          }
        },
        "systemStatus":[
          {
            "hostgroupId":"2",
            "severity":0,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"2",
            "severity":1,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"2",
            "severity":2,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"2",
            "severity":3,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"2",
            "severity":4,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"2",
            "severity":5,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"4",
            "severity":0,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"4",
            "severity":1,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"4",
            "severity":2,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"4",
            "severity":3,
            "numberOfTriggers":1
          },
          {
            "hostgroupId":"4",
            "severity":4,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"4",
            "severity":5,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"6",
            "severity":0,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"6",
            "severity":1,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"6",
            "severity":2,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"6",
            "severity":3,
            "numberOfTriggers":1
          },
          {
            "hostgroupId":"6",
            "severity":4,
            "numberOfTriggers":0
          },
          {
            "hostgroupId":"6",
            "severity":5,
            "numberOfTriggers":0
          }
        ],
        "hostStatus":[
          {
            "hostgroupId":"2",
            "numberOfGoodHosts":1,
            "numberOfBadHosts":0
          },
          {
            "hostgroupId":"4",
            "numberOfGoodHosts":0,
            "numberOfBadHosts":1
          },
          {
            "hostgroupId":"6",
            "numberOfGoodHosts":0,
            "numberOfBadHosts":1
          }
        ]
      }
    ],
    "numberOfGoodServers":0,
    "numberOfBadServers":1
  }
